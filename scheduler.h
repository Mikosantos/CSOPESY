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
    unsigned long long delayPerExec;
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

    std::vector<std::atomic<unsigned long long>> coreTicks; // one tick counter per core
    // std::atomic<int> systemTick{0};                      // used for batch generation

    std::vector<std::thread> tickThreads;

public:
    Scheduler(int cores, unsigned long long delay);
    virtual ~Scheduler();

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void schedulerLoop() = 0;
    virtual void coreWorker(int coreId) = 0;

    virtual void addProcess(const std::shared_ptr<Process>& proc);
    virtual int getBusyCoreCount() const;
    int getAvailableCoreCount() const;
    int getCPUTicks() const { return cpuTicks.load(); }
    
    // for ticks
    unsigned long long getCoreTick(int coreId) const {
        if (coreId >= 0 && coreId < coreTicks.size())
            return coreTicks[coreId].load();
        return -1;
    }

    void incrementCoreTick(int coreId) {
        if (coreId >= 0 && coreId < coreTicks.size())
            coreTicks[coreId]++;
    }

    /*
    This returns a list of currently running processes.
    Primarily used for logging purposes (in ConsolePanel's listProcesses, or report-util)
    Each core is individually locked for safe reading of assigned process without
    interfering with concurrent scheduling or execution.
    */
    virtual std::vector<std::shared_ptr<Process>> getRunningProcesses() const;


    // int getSystemTick() const {
    //     return systemTick.load();
    // }

    // void incrementSystemTick() {
    //     systemTick++;
    // }
};


