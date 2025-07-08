#pragma once  

#include "Scheduler.h"            
#include "FlatMemoryAllocator.h"
#include <condition_variable>     
#include <queue>                  
#include <mutex>                  
#include <unordered_set>          

class FlatMemoryAllocator;
// Round Robin Scheduler derived from base Scheduler
class RRScheduler : public Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;   // Queue of ready processes (FIFO order)
    unsigned long long quantumCycles;                  // Number of cycles allocated per process before switching
    std::condition_variable schedulerCV;               // Condition variable to signal scheduler loop
    std::mutex schedulerMutex;                         // Protects access to scheduler-related operations
    std::mutex assignLock;                             // Could be used for assigning cores safely (unused in .cpp)
    mutable std::mutex assignmentsMutex;               // Guards `coreAssignments` for thread-safe access

    std::vector<std::thread> coreThreads;                               // Threads that execute assigned processes
    std::vector<std::shared_ptr<Process>> coreAssignments;              // Tracks which process is assigned to which core
    std::unordered_set<std::shared_ptr<Process>> assignedProcesses;     // Tracks all assigned processes

    // Memory-related configuration
    unsigned long long maxOverallMem;                  
    unsigned long long memPerFrame;
    unsigned long long memPerProc;
    std::shared_ptr<FlatMemoryAllocator> memoryAllocator;

    std::vector<std::atomic<unsigned long long>> coreQuantumCounters;

public:
    // Constructor: initializes RR scheduler settings
    RRScheduler(int cores, int delay, unsigned long long quantum, 
                unsigned long long maxOverAllMemory, 
                unsigned long long memoryPerFrame, 
                unsigned long long memoryPerProcess,
                std::shared_ptr<FlatMemoryAllocator> allocator);

    // Core virtual methods from Scheduler base class
    void start() override;            // Initializes and launches scheduler and core threads
    void stop() override;             // Stops and joins all threads
    void schedulerLoop() override;    // The main RR loop that dispatches processes
    void coreWorker(int coreId) override; // Unused 
    void addProcess(const std::shared_ptr<Process>& proc) override; // Adds a new process to the ready queue

    // Getter methods for process state visualization
    int getBusyCoreCount() const override;
    std::vector<std::shared_ptr<Process>> getRunningProcesses() const override;
};
