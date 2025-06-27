#pragma once
#include "Scheduler.h"
#include <condition_variable>
#include <queue>
#include <mutex>

class RRScheduler : public Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    unsigned long long quantumCycles;
    std::condition_variable schedulerCV;
    std::mutex schedulerMutex;

public:
    RRScheduler(int cores, int delay, unsigned long long quantum);

    void start() override;
    void stop() override;
    void schedulerLoop() override;
    void coreWorker(int coreId) override;
    void addProcess(const std::shared_ptr<Process>& proc) override;
};
