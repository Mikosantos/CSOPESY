#include "FCFSScheduler.h"
#include <chrono>
#include <thread>

FCFSScheduler::FCFSScheduler(int cores, unsigned long long delay) : Scheduler(cores, delay) {}

FCFSScheduler::~FCFSScheduler() {
    stop();
}

// Start the scheduler and initialize CPU cores
void FCFSScheduler::start() {
    running = true;

    cores.reserve(coreCount);
    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&FCFSScheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);

    // tickThread = std::thread([this]() {
    //     while (running) {
    //         cpuTicks++;
    //         std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //     }
    // });
}

// Stop the scheduler and join all threads
void FCFSScheduler::stop() {
    running = false;

    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }

    if (schedulerThread.joinable()) schedulerThread.join();
    for (auto& core : cores) {
        if (core->thread.joinable()) core->thread.join();
    }
    // if (tickThread.joinable()) tickThread.join();
}

// Add a process to the ready queue
void FCFSScheduler::addProcess(const std::shared_ptr<Process>& proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(proc);
}

// Assigns processes to CPU cores (not busy) in a First-Come, First-Served manner
void FCFSScheduler::schedulerLoop() {
    while (running) {
        for (int i = 0; i < cores.size(); ++i) {
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
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// The core worker function that executes assigned processes 
void FCFSScheduler::coreWorker(int coreId) {
    auto& core = cores[coreId];

    while (running) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.wait(lock, [&]() {
            return core->assignedProcess != nullptr || !running;
        });

        if (!running) break;

        auto proc = core->assignedProcess;
        lock.unlock();

        while (running && proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {
            int currentTick = getCoreTick(coreId);
            
            // Simulate execution delay from SLEEP instruction
            if (proc->isSleeping(currentTick)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                incrementCoreTick(coreId);
                continue;
            }

            // NOTE: passing currentTick for SLEEP and FOR instruction
            proc->executeInstruction(coreId, currentTick);

            // Simulate execution delay from delayPerExec
            if (delayPerExec > 0) {
                for (int i = 0; i < delayPerExec; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    incrementCoreTick(coreId);
                }
            } else {
                incrementCoreTick(coreId);  // Only add 1 tick if no delay is set
            }
        }

        proc->setFinished(true);
        lock.lock();
        core->assignedProcess = nullptr;
        core->busy = false;
        lock.unlock();
    }
}