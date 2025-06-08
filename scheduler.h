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
    struct CPUCore {
        std::thread thread;
        std::mutex lock;
        std::condition_variable cv;
        std::shared_ptr<Process> assignedProcess = nullptr;
        bool busy = false;

        CPUCore() = default;
        ~CPUCore() = default;
    };

    std::vector<std::unique_ptr<CPUCore>> cores;
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::mutex queueMutex;

    std::atomic<bool> running = false;
    int coreCount;
    int delayPerExec;  // <-- Added: delay per instruction

    std::thread schedulerThread;

public:
    Scheduler(int cores = 4, int delay = 100);
    ~Scheduler();

    void start();
    void stop();
    void addProcess(const std::shared_ptr<Process>& proc);

private:
    void schedulerLoop();
    void coreWorker(int coreId);
};
