#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>
#include "Process.h"

class Scheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::vector<std::thread> cpuCores;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
    int coreCount;

public:
    Scheduler(int cores = 4);
    ~Scheduler();

    void start();
    void stop();
    void addProcess(const std::shared_ptr<Process>& proc);
    void runCore(int coreId);
};
