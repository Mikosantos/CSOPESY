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

        // generates a default constructor and destructor
        // to help with memory management
        CPUCore() = default;
        ~CPUCore() = default;
    };

    // List of CPU cores; each core has its own thread and can run a process independently
    std::vector<std::unique_ptr<CPUCore>> cores;

    // Ready queue for processes waiting to be executed
    std::queue<std::shared_ptr<Process>> readyQueue;

    // Mutex to protect access to the ready queue
    // This is necessary to ensure thread safety when multiple threads access the queue
    std::mutex queueMutex;

    // Atomic flag to indicate if the scheduler is running
    // atomic is used to ensure that the running state can be checked and modified safely across multiple threads
    std::atomic<bool> running = false;

    // Number of CPU cores available for scheduling; this is set during initialization and determines how many threads will be created
    int coreCount;
    int delayPerExec;

    // Thread that runs the scheduler loop; this thread is responsible for checking the ready queue and assigning processes to cores
    std::thread schedulerThread;

    //CPU TICKS
    std::atomic<int> cpuTicks = 0;
    std::thread tickThread;


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
