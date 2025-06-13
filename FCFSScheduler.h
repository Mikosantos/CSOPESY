#pragma once
#include "Scheduler.h"
#include <queue>

class FCFSScheduler : public Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;

public:
    FCFSScheduler(int cores, int delay);
    ~FCFSScheduler();

    void start() override;
    void stop() override;
    void schedulerLoop() override;
    void coreWorker(int coreId) override;
    void addProcess(const std::shared_ptr<Process>& proc) override;
};
