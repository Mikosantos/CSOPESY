#include "Scheduler.h"
#include <iostream>
#include <chrono>
#include <fstream>

Scheduler::Scheduler(int cores) : coreCount(cores) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    running = true;

    // Create and start each core
    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&Scheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    // Start scheduler dispatcher
    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
}

void Scheduler::stop() {
    running = false;

    // Wake all CPU cores
    for (auto& core : cores) {
        core->cv.notify_all();
    }

    // Join scheduler thread
    if (schedulerThread.joinable()) schedulerThread.join();

    // Join core threads
    for (auto& core : cores) {
        if (core->thread.joinable()) {
            core->thread.join();
        }
    }
}

void Scheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
}

void Scheduler::schedulerLoop() {
    while (running) {
        std::shared_ptr<Process> nextProc = nullptr;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!readyQueue.empty()) {
                nextProc = readyQueue.front();
            }
        }

        if (nextProc) {
            for (int i = 0; i < coreCount; ++i) {
                auto& core = cores[i];
                std::unique_lock<std::mutex> lock(core->lock);
                if (!core->busy && core->assignedProcess == nullptr) {
                    {
                        std::lock_guard<std::mutex> qLock(queueMutex);
                        readyQueue.pop(); // Remove from queue
                    }

                    core->assignedProcess = nextProc;
                    core->busy = true;
                    nextProc->setCoreNum(i);
                    core->cv.notify_one(); // Wake core thread
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Scheduler::coreWorker(int coreId) {
    auto& core = cores[coreId];

    while (running) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.wait(lock, [&]() {
            return core->assignedProcess != nullptr || !running;
        });

        if (!running) break;

        auto proc = core->assignedProcess;
        lock.unlock();

        while (proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {
            // Open file in append mode
            std::ofstream file(proc->getProcessName() + ".txt", std::ios::app);

            // Write timestamp, core, and message
            file << " Core: " << coreId
                 << "  \"Hello world from " << proc->getProcessName() << "!\"" << std::endl;

            // Advance instruction counter
            proc->setCompletedCommands(proc->getCompletedCommands() + 1);

            // cycle delay to simulate process execution
            // way of simulating "1 instruction takes 100 ms"
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // simulate execution
        }


        proc->setFinished(true);

        // Clear assignment and mark as idle
        lock.lock();
        core->assignedProcess = nullptr;
        core->busy = false;
    }
}
