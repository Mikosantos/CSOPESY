#pragma once
#include "Scheduler.h"
#include <condition_variable>
#include <queue>
#include <mutex>
#include <unordered_set>

class RRScheduler : public Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    unsigned long long quantumCycles;
    std::condition_variable schedulerCV;
    std::mutex schedulerMutex;
    std::mutex assignLock;
    mutable std::mutex assignmentsMutex; // Protect coreAssignments from race conditions

    std::vector<std::thread> coreThreads;
    std::vector<std::shared_ptr<Process>> coreAssignments;
    std::unordered_set<std::shared_ptr<Process>> assignedProcesses; // Track all assigned processes
public:
    RRScheduler(int cores, int delay, unsigned long long quantum);

    void start() override;
    void stop() override;
    void schedulerLoop() override;
    void coreWorker(int coreId) override;
    void addProcess(const std::shared_ptr<Process>& proc) override;
    
    // Override printing methods to use coreAssignments
    int getBusyCoreCount() const override;
    std::vector<std::shared_ptr<Process>> getRunningProcesses() const override;
};
