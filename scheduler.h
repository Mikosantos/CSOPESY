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
};
