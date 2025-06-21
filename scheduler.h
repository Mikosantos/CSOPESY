#pragma once

#include "Process.h"
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class Scheduler {
protected:
    int coreCount;
    int delayPerExec;
    std::atomic<bool> running{false};

    struct CPUCore {
        std::thread thread;
        std::shared_ptr<Process> assignedProcess;
        std::mutex lock;
        std::condition_variable cv;
        bool busy = false;
    };

    std::vector<std::unique_ptr<CPUCore>> cores;
    std::thread schedulerThread;
    std::thread tickThread;

    std::queue<std::shared_ptr<Process>> readyQueue;
    std::mutex queueMutex;

    std::atomic<int> cpuTicks{0};

    std::vector<std::atomic<int>> coreTicks; // one tick counter per core
    // std::atomic<int> systemTick{0};          // used for batch generation

public:
    Scheduler(int cores, int delay);
    virtual ~Scheduler();

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void schedulerLoop() = 0;
    virtual void coreWorker(int coreId) = 0;

    virtual void addProcess(const std::shared_ptr<Process>& proc);
    int getBusyCoreCount() const;
    int getAvailableCoreCount() const;
    int getCPUTicks() const { return cpuTicks.load(); }
    
    // for ticks
    int getCoreTick(int coreId) const {
        if (coreId >= 0 && coreId < coreTicks.size())
            return coreTicks[coreId].load();
        return -1;
    }

    void incrementCoreTick(int coreId) {
        if (coreId >= 0 && coreId < coreTicks.size())
            coreTicks[coreId]++;
    }

    // int getSystemTick() const {
    //     return systemTick.load();
    // }

    // void incrementSystemTick() {
    //     systemTick++;
    // }
};


