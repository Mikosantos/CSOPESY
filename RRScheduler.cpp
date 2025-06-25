#include "RRScheduler.h"
#include <chrono>
#include <thread>
#include <iostream>

// TODO: recheck test script,


// Constructor
RRScheduler::RRScheduler(int cores, int delay, unsigned long long quantum)
    : Scheduler(cores, delay), quantumCycles(quantum) {}

// Start the Round Robin scheduler
void RRScheduler::start() {
    running = true;

    // Initialize CPU cores and their tick threads
    cores.reserve(coreCount);
    tickThreads.resize(coreCount);

    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->busy = false;
        core->assignedProcess = nullptr;

        // Start core worker thread
        core->thread = std::thread(&RRScheduler::coreWorker, this, i);

        cores.push_back(std::move(core));

        // Start tick thread per core
        tickThreads[i] = std::thread([this, i]() {
            while (running) {
                incrementCoreTick(i);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // Start the scheduler thread
    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);
}

// Stop the scheduler
void RRScheduler::stop() {
    running = false;

    schedulerCV.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();

    // Notify and join core workers
    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }

    for (auto& core : cores) {
        if (core->thread.joinable()) core->thread.join();
    }

    // Join tick threads
    for (auto& t : tickThreads) {
        if (t.joinable()) t.join();
    }
}

/*
  This returns a list of currently running processes.
  Primarily used for logging purposes (in ConsolePanel's listProcesses, or report-util)
  Each core is individually locked for safe reading of assigned process without
  interfering with concurrent scheduling or execution.
*/
std::vector<std::shared_ptr<Process>> RRScheduler::getRunningProcesses() const {
    std::vector<std::shared_ptr<Process>> result;

    for (int i = 0; i < cores.size(); ++i) {
        std::shared_ptr<Process> proc;

        {
            std::lock_guard<std::mutex> lock(cores[i]->lock);
            proc = cores[i]->assignedProcess;

            if (proc) {
                result.push_back(proc);
            }
        }
    }

    return result;
}


void RRScheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
    schedulerCV.notify_one();
}

void RRScheduler::schedulerLoop() {
    std::unique_lock<std::mutex> schedLock(schedulerMutex);
    while (running) {
        // Wait for a process to be added to the ready queue or for the scheduler to stop
        // This will block until either a process is available or running is set to false
        schedulerCV.wait(schedLock, [this]() {
            return !readyQueue.empty() || !running;
        });

        /*
            Iterate through all CPU cores and assign processes from the ready queue
            to idle cores. If a core is busy or has an assigned process, it will skip that core.
            If a core is idle, it will take the next process from the ready queue and assign it to that core.
            The core worker thread will then be notified to start executing the assigned process.
        */
        for (int i = 0; i < cores.size(); ++i) {
            auto& core = cores[i];
            std::unique_lock<std::mutex> coreLock(core->lock);

            // If the core is not busy and has no assigned process, try to assign a new process
            // This check is done inside the core's lock to ensure thread safety
            if (!core->busy && core->assignedProcess == nullptr) {
                std::shared_ptr<Process> nextProc = nullptr;
                
                // Try to get the next process from the ready queue
                {
                    std::lock_guard<std::mutex> qLock(queueMutex);
                    if (!readyQueue.empty()) {
                        nextProc = readyQueue.front();
                        readyQueue.pop();
                    }
                }

                // If a process was found, assign it to the core
                if (nextProc) {
                    nextProc->setCoreNum(i);
                    core->assignedProcess = nextProc;
                    core->busy = true;
                    core->cv.notify_one();
                }
            }
        }
    }
}

/*
    Worker thread for each CPU core
    This method runs in a separate thread for each core and continuously checks for assigned processes.
    It executes instructions of the assigned process for a specified number of quantum cycles.
    If the process is finished or has no more instructions to execute, it will be re-enqueued to the ready queue.

    The worker will also handle sleeping processes by checking if the process is sleeping and waiting accordingly.
    If the process is not finished, it will be re-enqueued to the ready queue for further execution.
*/
void RRScheduler::coreWorker(int coreId) {
    auto& core = cores[coreId];

    while (running) {
        std::shared_ptr<Process> proc;

        {
            std::unique_lock<std::mutex> lock(core->lock);
            core->cv.wait(lock, [&]() {
                return core->assignedProcess != nullptr || !running;
            });

            if (!running) break;

            proc = core->assignedProcess;
        }

        // debugging purposes only
        if (!proc) {
            std::cerr << "[WARN] Core " << coreId << " received null process!\n";
            std::lock_guard<std::mutex> lock(core->lock);
            core->busy = false;
            continue;
        }

        unsigned long long executedTicks = 0;
        while (running && executedTicks < quantumCycles) {
            int currentTick = getCoreTick(coreId);
            if (proc->isFinished()) break;

            if (proc->isSleeping(currentTick)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            proc->executeInstruction(coreId, currentTick);
            executedTicks++;

            if (delayPerExec > 0) {
                for (int i = 0; i < delayPerExec; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }

        if (proc->getCompletedCommands() >= proc->getTotalNoOfCommands()) {
            proc->setFinished(true);
        } else {
            {
                std::lock_guard<std::mutex> qLock(queueMutex);
                readyQueue.push(proc);
                schedulerCV.notify_one();
            }
        }
        {
            std::lock_guard<std::mutex> lock(core->lock);
            proc->setCoreNum(-1);
            core->assignedProcess = nullptr;
            core->busy = false;
        }
    }
    

}
