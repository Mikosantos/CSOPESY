#pragma once
#include "Scheduler.h"

class FCFSScheduler : public Scheduler {
public:
    FCFSScheduler(int cores, unsigned long long delay);
    ~FCFSScheduler();

    void start() override;
    void stop() override;
    void schedulerLoop() override;
    void coreWorker(int coreId) override;
    void addProcess(const std::shared_ptr<Process>& proc) override;
};
