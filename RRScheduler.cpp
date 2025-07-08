#include "RRScheduler.h"
#include "FlatMemoryAllocator.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <unordered_set>

/**
 * @class RRScheduler
 * @brief Implements a Round Robin CPU scheduler with memory management integration.
 *
 * The RRScheduler schedules processes across multiple CPU cores using Round Robin logic.
 * It manages CPU core assignments, quantum expiration, and interacts with a memory allocator
 * to ensure that processes reside in memory before execution.
 */

 /**
 * @brief Constructs a new Round Robin Scheduler.
 *
 * Initializes internal data structures for managing CPU cores, tick counters, 
 * and integrates with a memory allocator for managing process memory allocation.
 *
 * @param cores Number of CPU cores available.
 * @param delay Milliseconds of delay per instruction execution.
 * @param quantum Number of ticks allowed per process per round.
 * @param maxOverAllMemory Maximum allowed memory in the system.
 * @param memoryPerFrame Size of one memory frame.
 * @param memoryPerProcess Memory required by a single process.
 * @param allocator Shared pointer to the FlatMemoryAllocator used to allocate memory.
 */
RRScheduler::RRScheduler(int cores, int delay, unsigned long long quantum, unsigned long long maxOverAllMemory, unsigned long long memoryPerFrame, unsigned long long memoryPerProcess, std::shared_ptr<FlatMemoryAllocator> allocator) :
    Scheduler(cores, delay), quantumCycles(quantum), maxOverallMem(maxOverAllMemory), memPerFrame(memoryPerFrame), memPerProc(memoryPerProcess), memoryAllocator(allocator), coreQuantumCounters(cores) 
    {
        for (int i = 0; i < cores; ++i)
            coreQuantumCounters[i].store(0);
    }


/**
 * @brief Starts the Round Robin scheduler system.
 *
 * Spawns a thread for each core to simulate ticking, and a main thread that
 * handles scheduling logic based on process readiness and quantum expiration.
 */
void RRScheduler::start() {
    running = true;

    // Resize vectors to number of CPU cores
    cores.resize(coreCount);          // Inherited from Scheduler
    tickThreads.resize(coreCount);    // Tracks tick counter per core
    coreThreads.resize(coreCount);    // One thread per core for executing processes
    coreAssignments.resize(coreCount, nullptr); // No process assigned initially

    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();  // CPUCore is a struct or class with state
        core->busy = false;
        core->assignedProcess = nullptr;
        cores[i] = std::move(core);
    }

    // Launch "tick" thread per core (simulates CPU tick)
    for (int i = 0; i < coreCount; ++i) {
        tickThreads[i] = std::thread([this, i]() {
            while (running) {
                incrementCoreTick(i);  // Update logical clock for this core
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Tick interval
            }
        });
    }

    // Launch scheduler loop thread (runs RR logic)
    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);
}


/**
 * @brief Stops the scheduler gracefully.
 *
 * Ensures all threads (scheduler, core, and tick threads) are joined.
 * Cleans up core assignments and resets scheduler state.
 */
void RRScheduler::stop() {
    running = false;

    schedulerCV.notify_all();   // Wake up scheduler loop
    if (schedulerThread.joinable()) schedulerThread.join();

    // Notify all cores
    // Join core threads - use the coreThreads vector from schedulerLoop
    // Note: We need to ensure all threads are properly joined before cleanup
    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }
    // Join core threads
    for (auto& t : coreThreads) {
        if (t.joinable()) t.join();
    }

    // Join tick threads
    for (auto& t : tickThreads) {
        if (t.joinable()) t.join();
    }

    if (schedulerThread.joinable()) {
        schedulerThread.join();
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
    proc->setProcessState(ProcessState::READY);
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);  // FIFO order
    }
    schedulerCV.notify_one();   // Wake scheduler thread
}

/**
 * @brief The main Round Robin scheduling loop.
 *
 * Continuously checks for:
 *   1. Finished processes — deallocates them from memory.
 *   2. Quantum-expired processes — resets and requeues them.
 *   3. Idle cores — assigns ready processes if memory permits.
 *
 * Runs until the scheduler is marked as stopped. Periodically triggers memory visualization.
 * Main scheduler loop that runs while the system is marked as running.
 */
void RRScheduler::schedulerLoop() {
    static int quantumExpiryCounter = 0;
    const int LOG_INTERVAL = 4; 
    while (running) {
         // Lock the scheduler to ensure only one thread can manipulate shared state
        std::unique_lock<std::mutex> schedLock(schedulerMutex);     
        // Wait for up to 1 millisecond or until a new process is added or scheduler is stopped        
        schedulerCV.wait_for(schedLock, std::chrono::milliseconds(1), [this]() {
             // Only proceed if there's at least one process ready or scheduler is no longer running
            return !readyQueue.empty() || !running;
        });
        // Iterate through all CPU cores
        for (int core = 0; core < coreCount; ++core) {
             // ========== CASE 1: PROCESS FINISHED ========== //
                // If a process is assigned to this core and it has completed all instructions
                // Join finished thread if process is finished
            if (coreAssignments[core] && coreAssignments[core]->isFinished()) {
                // If there's an active thread running the process on this core, wait for it to finish
                if (coreThreads[core].joinable()) {
                    coreThreads[core].join();
                }
                if (memoryAllocator->isProcessAllocated(coreAssignments[core])) {
                    memoryAllocator->deallocate(coreAssignments[core]);
                    //std::cout << "[Allocator] Deallocated memory for " << coreAssignments[core]->getProcessName() << "\n";
                }
                 // Update core and scheduler states
                {
                    std::lock_guard<std::mutex> lock(cores[core]->lock);    // Lock this specific core's state
                    cores[core]->busy = false;                              // Mark core as no longer busy
                    coreAssignments[core]->setCoreNum(-1);                  // Detach process from this core
                    coreAssignments[core]->setProcessState(ProcessState::FINISHED); //Set process state as finished
                    coreThreads[core] = std::thread();                      // Reset thread slot
                    coreAssignments[core] = nullptr;                        // Unassign process from this core
                }
                continue;   // Skip to next core
            }
            // ========== CASE 2: QUANTUM EXPIRED ========== //
                // If the process has used up its allowed quantum (but not finished yet)
                // Join thread if quantum exceeded but not finished
            if (coreAssignments[core] &&
                coreAssignments[core]->getQuantumUsed() >= quantumCycles) {
                
                // Pause the process execution thread
                if (coreThreads[core].joinable()) {
                    coreThreads[core].join();
                }
                
                // Reset the process's quantum usage, requeue
                coreAssignments[core]->resetQuantumUsed();
                // Mark the process as READY again so it can be rescheduled
                coreAssignments[core]->setProcessState(ProcessState::READY); 

                {   // Safely return the process to the ready queue
                    std::lock_guard<std::mutex> qLock(queueMutex);
                    readyQueue.push(coreAssignments[core]);
                }
                // Reset the core's thread and assignment
                coreThreads[core] = std::thread(); // Reset
                coreAssignments[core] = nullptr;
                {
                    std::lock_guard<std::mutex> lock(cores[core]->lock);
                    cores[core]->busy = false;
                }
                // Notify the scheduler in case it’s waiting
                schedulerCV.notify_one();

                ++quantumExpiryCounter;
                if (quantumExpiryCounter % LOG_INTERVAL == 0) {
                    memoryAllocator->displayMemoryPerCycle(quantumExpiryCounter);
                }

                continue;
            }
            
            
             // ========== CASE 3: CORE IS IDLE ========== //
                // If this core is idle (i.e., not currently executing any process)
                // If core is idle, assign a new process
            if (!coreAssignments[core] && !cores[core]->busy) {
                std::shared_ptr<Process> nextProc = nullptr;
                {   // Try to fetch the next ready process from the queue
                    std::lock_guard<std::mutex> qLock(queueMutex);
                    if (!readyQueue.empty()) {
                        nextProc = readyQueue.front();
                        readyQueue.pop();
                    }
                }

                if (nextProc && !memoryAllocator->isProcessAllocated(nextProc)) {
                        if (!memoryAllocator->allocate(nextProc)) {
                            // Requeue if memory full
                            //std::cout << "[Allocator] Failed to allocate memory for " << nextProc->getProcessName() << "\n";
                            std::lock_guard<std::mutex> qLock(queueMutex);
                            readyQueue.push(nextProc);
                            continue;
                        } else {
                            //std::cout << "[Allocator] Successfully allocated memory for " << nextProc->getProcessName() << "\n";
                        }
                    }

                 // If we found a process to run
                if (nextProc) {

                    coreAssignments[core] = nextProc;   // Assign the process to this core
                    nextProc->setCoreNum(core);         // Set the core ID in the process 
                    nextProc->setProcessState(ProcessState::RUNNING);    // Update process state to RUNNING
                    
                    {   // Mark the core as busy so it won’t be re-assigned
                        std::lock_guard<std::mutex> lock(cores[core]->lock);
                        cores[core]->busy = true;
                    }
                    // Reset the quantum usage counter before starting execution
                    nextProc->resetQuantumUsed();
                     // Create a new thread to run the process logic
                    coreThreads[core] = std::thread([this, nextProc, core]() {
                        // Count ticks used by this process
                        unsigned long long ticks = 0; 
                         // Keep executing while:
                        // - The scheduler is running
                        // - The process is not finished
                        // - The process hasn't exceeded its quantum  
                        while (running && !nextProc->isFinished() && ticks < quantumCycles) {
                            // Get the current tick count for this core
                            int tick = getCoreTick(core);
                            // If the process is sleeping during this tick 
                            if (nextProc->isSleeping(tick)) {
                                nextProc->setProcessState(ProcessState::WAITING);
                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                continue;
                            }
                            
                            nextProc->setProcessState(ProcessState::RUNNING);   // Set process state to RUNNING (might have been WAITING previously)
                            nextProc->executeInstruction(core, tick);           // Execute one instruction of the process at this tick
                            nextProc->incrementQuantumUsed();                   // Increment the process’s quantum usage
                            ++ticks;

                            if (delayPerExec > 0) {                              //  Delay 
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
     // ======= Final Cleanup: Join any leftover core threads ======= //
    // Final join on all threads when stopping
    for (int i = 0; i < coreCount; ++i) {
        if (coreThreads[i].joinable()) coreThreads[i].join();
    }
}


void RRScheduler::coreWorker(int coreId) {
    // to work with scheduler base class
}
/**
 * @brief Returns the number of CPU cores currently running non-finished processes.
 * 
 * Override printing methods to use coreAssignments.
 * 
 * @return Number of busy CPU cores.
 */
int RRScheduler::getBusyCoreCount() const {
    int count = 0;
    for (int i = 0; i < coreCount; ++i) {
        if (coreAssignments[i] != nullptr && !coreAssignments[i]->isFinished()) {
            count++;
        }
    }
    return count;
}

/**
 * @brief Retrieves all processes currently assigned to cores and not yet completed.
 *
 *
 * @return A vector of shared pointers to running processes.
 */
std::vector<std::shared_ptr<Process>> RRScheduler::getRunningProcesses() const {
    std::vector<std::shared_ptr<Process>> result;
    
    for (int i = 0; i < coreCount; ++i) {
        if (coreAssignments[i] && 
            coreAssignments[i]->getCompletedCommands() < coreAssignments[i]->getTotalNoOfCommands()) {
            result.push_back(coreAssignments[i]);
        }
    }
    
    return result;
}