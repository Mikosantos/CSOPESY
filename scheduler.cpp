#include "Scheduler.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <time.h>
#include <thread>
#include <sstream>
#include <filesystem>

// constructor for the Scheduler class
Scheduler::Scheduler(int cores, int delay)
    : coreCount(cores), delayPerExec(delay) {}

Scheduler::~Scheduler() {
    stop();
}

// in threading, joining a thread (.join) means waiting for it to finish execution before proceeding.

// Starts the scheduler and creates threads for each CPU core
// 1 thread per CPU core and 1 thread for the scheduler itself
void Scheduler::start() {
    running = true;

    // Create and start one thread per CPU core
    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&Scheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    // Start the scheduler thread
    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);

    // Start the CPU tick thread to simulate CPU ticks
    tickThread = std::thread([this]() {
        while (running) {
            cpuTicks++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

}

// Stops the scheduler and joins all threads
void Scheduler::stop() {
    running = false;

    // Wake all threads in case they're waiting
    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }

    // Wait for the scheduler thread to finish
    if (schedulerThread.joinable()) schedulerThread.join();

    // Wait for all core threads to finish
    for (auto& core : cores) {
        if (core->thread.joinable()) {
            core->thread.join();
        }
    }

    // Wait for the CPU tick thread to finish
    if (tickThread.joinable()) tickThread.join();
}

// Adds a process to the ready queue
void Scheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
}

// Main scheduler loop that checks for ready processes and assigns them to cores
void Scheduler::schedulerLoop() {
    while (running) {
        // look for an available core
        for (int i = 0; i < coreCount; ++i) {
            auto& core = cores[i];
            std::unique_lock<std::mutex> coreLock(core->lock);

            if (!core->busy && core->assignedProcess == nullptr) {
                std::shared_ptr<Process> nextProc = nullptr;
                {
                    std::lock_guard<std::mutex> qLock(queueMutex);
                    if (!readyQueue.empty()) {
                        nextProc = readyQueue.front();
                        readyQueue.pop();
                    }
                }

            if (nextProc) {
                core->assignedProcess = nextProc;
                core->busy = true;
                nextProc->setCoreNum(i);
                core->cv.notify_one();
                break;
                }
            }
        }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// responsible for executing the process on a specific core
void Scheduler::coreWorker(int coreId) {
    auto& core = cores[coreId];

    while (running) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.wait(lock, [&]() {
            return core->assignedProcess != nullptr || !running;
        });

        if (!running) break;

        auto proc = core->assignedProcess;
        lock.unlock();

        // create directory for logs (if not yet exists)
        std::string logsDir = "processLogs";
        std::error_code err;

        if (!std::filesystem::exists(logsDir)) {
            std::filesystem::create_directory(logsDir);
        }

        // Construct file path with logs directory
        std::string logFilePath = logsDir + "/" + proc->getProcessName() + ".txt";

        // write to file
        std::ofstream file(logFilePath, std::ios::app);
        file << "Process name: " << proc->getProcessName() 
             << "\nLogs: \n\n";

        while (proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {

            // Generate current time (per instruction)
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm local_time;
            localtime_s(&local_time, &now_c);

            std::ostringstream timestamp;
            timestamp << "("
                    << std::setw(2) << std::setfill('0') << local_time.tm_mon + 1 << "/"
                    << std::setw(2) << std::setfill('0') << local_time.tm_mday << "/"
                    << (local_time.tm_year + 1900) << " ";

            int hour = local_time.tm_hour;
            std::string ampm = "AM";
            if (hour >= 12) {
                ampm = "PM";
                if (hour > 12) hour -= 12;
            }
            if (hour == 0) hour = 12;

            timestamp << std::setw(2) << std::setfill('0') << hour << ":"
                      << std::setw(2) << std::setfill('0') << local_time.tm_min << ":"
                      << std::setw(2) << std::setfill('0') << local_time.tm_sec << " "
                      << ampm << ")";

            // Write full line with real-time timestamp
            file << timestamp.str() << " Core: " << coreId << "  \"Hello world from " << proc->getProcessName() << "!\"" << std::endl;
            file.flush();

            proc->setCompletedCommands(proc->getCompletedCommands() + 1);
            // std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));

            int startTick = cpuTicks.load();
            while (cpuTicks.load() - startTick < delayPerExec) {

            }
            // TODO: not exiting properly if theres a running process
        }

        proc->setFinished(true);

        lock.lock();
        core->busy = false;
        core->assignedProcess = nullptr;
        lock.unlock();
    }
}



// getters

int Scheduler::getBusyCoreCount() const {
    int count = 0;
    for (const auto& core : cores) {
        std::lock_guard<std::mutex> lock(core->lock);
        if (core->busy) {
            count++;
        }
    }
    return count;
}

int Scheduler::getAvailableCoreCount() const {
    return coreCount - getBusyCoreCount();
}