#include "Scheduler.h"
#include "InstructionUtils.h"

#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <time.h>
#include <thread>
#include <sstream>
#include <filesystem>

// constructor for the Scheduler class
Scheduler::Scheduler(int cores, int delay)
    : coreCount(cores), delayPerExec(delay) {}

Scheduler::~Scheduler() {
    stop();
}

// in threading, joining a thread (.join) means waiting for it to finish execution before proceeding.

// Starts the scheduler and creates threads for each CPU core
// 1 thread per CPU core and 1 thread for the scheduler itself
void Scheduler::start() {
    running = true;

    // Create and start one thread per CPU core
    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&Scheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    // Start the scheduler thread
    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);

    // Start the CPU tick thread to simulate CPU ticks
    tickThread = std::thread([this]() {
        while (running) {
            cpuTicks++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

}

// Stops the scheduler and joins all threads
void Scheduler::stop() {
    running = false;

    // Wake all threads in case they're waiting
    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }

    // Wait for the scheduler thread to finish
    if (schedulerThread.joinable()) schedulerThread.join();

    // Wait for all core threads to finish
    for (auto& core : cores) {
        if (core->thread.joinable()) {
            core->thread.join();
        }
    }

    // Wait for the CPU tick thread to finish
    if (tickThread.joinable()) tickThread.join();
}

// Adds a process to the ready queue
void Scheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
}

// Main scheduler loop that checks for ready processes and assigns them to cores
void Scheduler::schedulerLoop() {
    while (running) {
        // look for an available core
        for (int i = 0; i < coreCount; ++i) {
            auto& core = cores[i];
            std::unique_lock<std::mutex> coreLock(core->lock);

            if (!core->busy && core->assignedProcess == nullptr) {
                std::shared_ptr<Process> nextProc = nullptr;
                {
                    std::lock_guard<std::mutex> qLock(queueMutex);
                    if (!readyQueue.empty()) {
                        nextProc = readyQueue.front();
                        readyQueue.pop();
                    }
                }

            if (nextProc) {
                core->assignedProcess = nextProc;
                core->busy = true;
                nextProc->setCoreNum(i);
                core->cv.notify_one();
                break;
                }
            }
        }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// responsible for executing the process on a specific core
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

        // execution loop for the process
        while (running && proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {
            if (proc->isSleeping(cpuTicks.load())) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            proc->executeInstruction(coreId, cpuTicks.load());

            // Simulate delayPerExec
            int startTick = cpuTicks.load();
            while (running && (cpuTicks.load() - startTick < delayPerExec)) {
                // std::this_thread::sleep_for(std::chrono::microseconds(50));
            }

        }

        proc->setFinished(true);
        lock.lock();
        core->assignedProcess = nullptr;
        core->busy = false;
        lock.unlock();
    }
}




// getters for printing system summary
int Scheduler::getBusyCoreCount() const {
    int count = 0;
    for (const auto& core : cores) {
        std::lock_guard<std::mutex> lock(core->lock);
        if (core->busy) {
            count++;
        }
    }
    return count;
}

int Scheduler::getAvailableCoreCount() const {
    return coreCount - getBusyCoreCount();
}