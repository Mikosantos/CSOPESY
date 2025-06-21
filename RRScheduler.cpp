#include "RRScheduler.h"
#include <chrono>
#include <thread>
#include <iostream>

// Constructor for RRScheduler
RRScheduler::RRScheduler(int cores, int delay, int quantum)
    : Scheduler(cores, delay), quantumCycles(quantum) {}

// Start the Round Robin scheduler
void RRScheduler::start() {
    running = true;

    // Initialize CPU cores
    cores.reserve(coreCount);
    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->busy = false;
        core->assignedProcess = nullptr;
        core->thread = std::thread(&RRScheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    // Start the scheduler thread
    // Responsible for managing the ready queue and assigning processes to cores
    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);

    // Start the tick thread
    // Responsible for simulating CPU ticks
    // tickThread = std::thread([this]() {
    //     while (running) {
    //         cpuTicks++;
    //         std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //     }
    // });
}

// Stop the Round Robin scheduler
void RRScheduler::stop() {
    running = false;

    // Notify (wake up) the scheduler thread to wake up and exit
    schedulerCV.notify_all();
    if (schedulerThread.joinable()) schedulerThread.join();

    // Notify all core workers to wake up and exit
    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }

    // Join all core worker threads
    // This ensures that all threads finish execution before the scheduler is destroyed
    for (auto& core : cores) {
        if (core->thread.joinable()) core->thread.join();
    }

    // Join the tick thread
    // This ensures that the tick thread finishes execution before the scheduler is destroyed
    // if (tickThread.joinable()) tickThread.join();
}

/*
    Add a process to the ready queue
    This method is called by the main program to enqueue a new process
    It calls the scheduler's condition variable to wake up the scheduler thread and
    notify it that a new process is available for scheduling.

    lock (mutex) is used to ensure thread safety when accessing the ready queue.
*/
void RRScheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
    schedulerCV.notify_one();
}

/*
    The main loop of the Round Robin scheduler
    This method runs in a separate thread and continuously checks the ready queue for processes.
    It assigns processes to idle CPU cores and notifies the core worker threads to start execution (of instructions inside processes).

    The loop will run until the scheduler is stopped (running == false)
    It uses a condition variable to wait for new processes to be added to the ready queue.
*/
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
                    core->assignedProcess = nextProc;
                    nextProc->setCoreNum(i);
                    core->cv.notify_one(); // Tell worker to start
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

        // Wait for an assigned process
        {
            std::unique_lock<std::mutex> lock(core->lock);
            // Wait until a process is assigned or the scheduler is stopped
            core->cv.wait(lock, [&]() {
                return core->assignedProcess != nullptr || !running;
            });

            if (!running) break;

            // If a process is assigned, take it and mark the core as busy
            proc = core->assignedProcess;
            core->assignedProcess = nullptr;  // clear it safely inside lock
            core->busy = true;
        }

        // DEBUGGING; In case of null process, log a warning and skip this cycle
        if (!proc) {
            std::cerr << "[WARN] Core " << coreId << " received null process!\n";
            std::lock_guard<std::mutex> lock(core->lock);
            core->busy = false;
            continue;  // skip this cycle if null
        }
        //

        // Execute the assigned process for a specified number of quantum cycles (RR behavior)
        int executedTicks = 0;
        while (running && executedTicks < quantumCycles) {
            int currentTick = getCoreTick(coreId);

            if (proc->isFinished()) break;

            // Check if the process is sleeping
            // cpuTicks.load() is used to get the current CPU ticks
            if (proc->isSleeping(currentTick)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                incrementCoreTick(coreId);
                break;
            }

            // Execute the instruction of the process
            proc->executeInstruction(coreId, currentTick);
            executedTicks++; // Increment the executed ticks for this quantum cycle (needs checking to see if it works properly)

            // Delays when executing instructions
            if (delayPerExec > 0) {
                int startTick = currentTick;
                // this condition will wait until the specified delay per execution is reached
                while (running && (cpuTicks.load() - startTick < delayPerExec)) {
                    std::this_thread::sleep_for(std::chrono::microseconds(50)); 
                }
            }
            incrementCoreTick(coreId);
        }

        // After executing the quantum cycles, check if the process is finished
        // If the process has completed all its commands, mark it as finished
        // If not finished, re-enqueue it to the ready queue for further execution
        if (proc->getCompletedCommands() >= proc->getTotalNoOfCommands()) {
            proc->setFinished(true);
        } else {
            // If the process is not finished, re-enqueue it to the ready queue
            std::lock_guard<std::mutex> qLock(queueMutex);
            readyQueue.push(proc);
            schedulerCV.notify_one(); // notify scheduler
        }
        // Mark the core as not busy after processing the assigned process
        // This allows the core to accept new processes in the next cycle
        {
            std::lock_guard<std::mutex> lock(core->lock);
            core->busy = false;
        }

        // Reset core number which means it is no longer assigned to any core
        // This means its core number is set to -1, indicating it is not currently running on any core
        proc->setCoreNum(-1); 
    }
}

