#pragma once
#include "Scheduler.h"

class FCFSScheduler : public Scheduler {
public:
    FCFSScheduler(int cores, unsigned long long delay, const Config& config, std::shared_ptr<MemoryManager> memManager);
    ~FCFSScheduler();

    void start() override;
    void stop() override;
    void schedulerLoop() override;
    void coreWorker(int coreId) override;
    void addProcess(const std::shared_ptr<Process>& proc) override;

    uint64_t getTotalCpuTicks() const override {
        uint64_t sum = 0;
        for (auto t : totalTicksPerCore) sum += t;
        return sum;
    }

    uint64_t getActiveCpuTicks() const override {
        uint64_t sum = 0;
        for (auto t : activeTicksPerCore) sum += t;
        return sum;
    }

    uint64_t getIdleCpuTicks() const override {
        return getTotalCpuTicks() - getActiveCpuTicks();
    }

private:
    Config config;
    std::shared_ptr<MemoryManager> memoryManager;

    std::vector<uint64_t> totalTicksPerCore;
    std::vector<uint64_t> activeTicksPerCore;
};
