#include "RRScheduler.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <unordered_set>

// Constructor
RRScheduler::RRScheduler(int cores, int delay, unsigned long long quantum, const Config& config, std::shared_ptr<MemoryManager> memManager)
    : Scheduler(cores, delay), quantumCycles(quantum), config(config), memoryManager(memManager) {}

// Start the Round Robin scheduler
void RRScheduler::start() {
    running = true;

    // Prepare CPU cores
    cores.resize(coreCount);
    tickThreads.resize(coreCount);

    totalTicksPerCore.resize(coreCount, 0);
    activeTicksPerCore.resize(coreCount, 0);

    // core assignments
    coreThreads.resize(coreCount);
    coreAssignments.resize(coreCount, nullptr);

    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->busy = false;
        core->assignedProcess = nullptr;
        cores[i] = std::move(core);
    }

    // Start tick threads
    for (int i = 0; i < coreCount; ++i) {
        tickThreads[i] = std::thread([this, i]() {
            while (running) {
                incrementCoreTick(i);
                totalTicksPerCore[i]++;

                if (coreAssignments[i] && cores[i]->busy) {
                    activeTicksPerCore[i]++;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);
}

// Stop the scheduler
void RRScheduler::stop() {
    running = false;

    schedulerCV.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();

    // Join core threads - use the coreThreads vector from schedulerLoop
    // Note: We need to ensure all threads are properly joined before cleanup
    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }

    for (auto& t : coreThreads) {
        if (t.joinable()) t.join();
    }

    // Join tick threads
    for (auto& t : tickThreads) {
        if (t.joinable()) t.join();
    }

    // additional cleanup
    for (int i = 0; i < coreCount; ++i) {
        coreAssignments[i] = nullptr;
        cores[i]->assignedProcess = nullptr;
        cores[i]->busy = false;
    }
}

// Add process to ready queue
void RRScheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
    schedulerCV.notify_one();
}

void RRScheduler::schedulerLoop() {
    while (running) {
        std::unique_lock<std::mutex> schedLock(schedulerMutex);
        schedulerCV.wait_for(schedLock, std::chrono::milliseconds(1), [this]() {
            return !readyQueue.empty() || !running;
        });

        for (int core = 0; core < coreCount; ++core) {
            // Join finished thread if process is finished
            {
                std::lock_guard<std::mutex> lock(cores[core]->lock);
                if (coreAssignments[core] && coreAssignments[core]->isFinished()) {
                    if (coreThreads[core].joinable()) {
                        coreThreads[core].join();
                    }

                    coreAssignments[core]->setCoreNum(-1);
                    memoryManager->deallocateProcess(coreAssignments[core]->getProcessNo());

                    coreThreads[core] = std::thread(); // Reset
                    coreAssignments[core] = nullptr;
                    cores[core]->busy = false;

                    continue;
                }
            }

            // Join thread if quantum exceeded but not finished
            if (coreAssignments[core] &&
                coreAssignments[core]->getQuantumUsed() >= quantumCycles) {
                if (coreThreads[core].joinable()) {
                    coreThreads[core].join();
                }

                // Reset quantum, requeue
                coreAssignments[core]->resetQuantumUsed();
                {
                    std::lock_guard<std::mutex> qLock(queueMutex);
                    readyQueue.push(coreAssignments[core]);
                }
                coreThreads[core] = std::thread(); // Reset
                coreAssignments[core] = nullptr;
                {
                    std::lock_guard<std::mutex> lock(cores[core]->lock);
                    cores[core]->busy = false;
                }
                schedulerCV.notify_one();
            }

            // If core is idle, assign a new process
            if (!coreAssignments[core] && !cores[core]->busy) {
                std::shared_ptr<Process> nextProc = nullptr;
                {
                    std::lock_guard<std::mutex> qLock(queueMutex);
                    if (!readyQueue.empty()) {
                        nextProc = readyQueue.front();
                        readyQueue.pop();
                    }
                }

                if (nextProc) {
                    coreAssignments[core] = nextProc;
                    nextProc->setCoreNum(core);
                    {
                        std::lock_guard<std::mutex> lock(cores[core]->lock);
                        cores[core]->busy = true;
                    }

                    nextProc->resetQuantumUsed();

                    /*
                        This is the first time we are initializing the process's memory.
                    */
                    if (!nextProc->isMemoryInitialized()) {

                        // Try to allocate memory; if not enough, requeue and skip this core cycle
                        if (!memoryManager->allocateProcess(nextProc->getProcessNo(), nextProc->getMemSize())) {
                            {
                                std::lock_guard<std::mutex> qLock(queueMutex);
                                readyQueue.push(nextProc);
                            }

                            {
                                std::lock_guard<std::mutex> lock(cores[core]->lock);
                                cores[core]->busy = false;
                            }

                            coreAssignments[core] = nullptr;
                            continue;
                        }

                        nextProc->initializePages(config.memPerFrame);
                        nextProc->markMemoryInitialized(); 
                    }

                    coreThreads[core] = std::thread([this, nextProc, core]() {
                        unsigned long long ticks = 0;
                        while (running && !nextProc->isFinished() && ticks < quantumCycles) {
                            int tick = getCoreTick(core);

                            if (nextProc->isSleeping(tick)) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                continue;
                            }


                            // nextProc->executeInstruction(core, tick);

                            // Check if instruction failed due to memory
                            if (!nextProc->executeInstruction(core, tick)) {
                                if (nextProc->hasMemoryViolation()) {
                                    // debugging
                                    // std::cerr << "[RR] Memory violation in " << nextProc->getProcessName()
                                    //         << " at 0x" << std::hex << nextProc->getViolationAddress() << "\n";
                                    nextProc->setFinished(true);
                                } else {
                                    std::lock_guard<std::mutex> qLock(queueMutex);
                                    readyQueue.push(nextProc);
                                }

                                std::lock_guard<std::mutex> lock(cores[core]->lock);
                                cores[core]->busy = false;
                                coreAssignments[core] = nullptr;
                                return;
                            }

                            nextProc->incrementQuantumUsed();
                            ++ticks;

                            if (delayPerExec > 0) {
                                for (int i = 0; i < delayPerExec; ++i) {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                }
                            }
                        }
                    });
                }
            }
        }
    }

    // Final join on all threads when stopping
    for (int i = 0; i < coreCount; ++i) {
        if (coreThreads[i].joinable()) coreThreads[i].join();
    }
}


void RRScheduler::coreWorker(int coreId) {
    // to work with scheduler base class
}

// Override printing methods to use coreAssignments
int RRScheduler::getBusyCoreCount() const {
    int count = 0;
    for (int i = 0; i < coreCount; ++i) {
        if (coreAssignments[i] != nullptr && 
            coreAssignments[i]->isMemoryInitialized() &&
            !coreAssignments[i]->isFinished()) {
            count++;
        }
    }
    return count;
}

std::vector<std::shared_ptr<Process>> RRScheduler::getRunningProcesses() const {
    std::vector<std::shared_ptr<Process>> result;
    
    for (int i = 0; i < coreCount; ++i) {
        if (coreAssignments[i] && 
            coreAssignments[i]->isMemoryInitialized() && // new
            coreAssignments[i]->getCompletedCommands() < coreAssignments[i]->getTotalNoOfCommands()) {
            result.push_back(coreAssignments[i]);
        }
    }
    
    return result;
}

std::shared_ptr<Process> RRScheduler::getProcessOnCore(int coreId) const {
    if (coreId < 0 || coreId >= coreCount) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(assignmentsMutex);
    return coreAssignments[coreId];
}

std::vector<std::shared_ptr<Process>> RRScheduler::getReadyQueueSnapshot() const {
    std::vector<std::shared_ptr<Process>> snapshot;
    std::lock_guard<std::mutex> lock(queueMutex);
    
    std::queue<std::shared_ptr<Process>> tempQueue = readyQueue;

    while (!tempQueue.empty()) {
        snapshot.push_back(tempQueue.front());
        tempQueue.pop();
    }
    return snapshot;
}