#include "Scheduler.h"
#include <iostream>
#include <chrono>
#include <fstream>

Scheduler::Scheduler(int cores, int delay)
    : coreCount(cores), delayPerExec(delay) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    running = true;

    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&Scheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
}

void Scheduler::stop() {
    running = false;

    for (auto& core : cores) {
        core->cv.notify_all();
    }

    if (schedulerThread.joinable()) schedulerThread.join();

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
                        readyQueue.pop();
                    }

                    core->assignedProcess = nextProc;
                    core->busy = true;
                    nextProc->setCoreNum(i);
                    core->cv.notify_one();
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
            // Simulate instruction execution
            std::ofstream file(proc->getProcessName() + ".txt", std::ios::app);
            file << " Core: " << coreId
                 << "  \"Hello world from " << proc->getProcessName() << "!\"" << std::endl;

            proc->setCompletedCommands(proc->getCompletedCommands() + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));  // <-- Uses config value
        }

        proc->setFinished(true);

        lock.lock();
        core->assignedProcess = nullptr;
        core->busy = false;
    }
}
