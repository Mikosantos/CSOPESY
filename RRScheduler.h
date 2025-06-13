#pragma once
#include "Scheduler.h"
#include <queue>

class RRScheduler : public Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    int quantumCycles;

public:
    RRScheduler(int cores, int delay, int quantum);

    void start() override;
    void stop() override;
    void schedulerLoop() override;
    void coreWorker(int coreId) override;
    void addProcess(const std::shared_ptr<Process>& proc) override;
};
