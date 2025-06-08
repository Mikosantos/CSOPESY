#include "Scheduler.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <time.h>
#include <thread>
#include <sstream>

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
}

// Stops the scheduler and joins all threads
void Scheduler::stop() {
    running = false;

    // Wake all threads in case they're waiting
    for (auto& core : cores) {
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
        std::shared_ptr<Process> nextProc = nullptr;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!readyQueue.empty()) {
                nextProc = readyQueue.front();
            }
        }

        if (nextProc) {
            for (int i = 0; i < coreCount; ++i) {
                auto& core = cores[i];
                std::unique_lock<std::mutex> lock(core->lock);
                if (!core->busy && core->assignedProcess == nullptr) {
                    {
                        std::lock_guard<std::mutex> qLock(queueMutex);
                        readyQueue.pop();
                    }

                    core->assignedProcess = nextProc;
                    core->busy = true;
                    nextProc->setCoreNum(i);
                    core->cv.notify_one();
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

        // write to file
        while (proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {
            std::ofstream file(proc->getProcessName() + ".txt", std::ios::app);

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
            file << timestamp.str()
                << " Core: " << coreId
                << "  \"Hello world from " << proc->getProcessName() << "!\"" << std::endl;

            proc->setCompletedCommands(proc->getCompletedCommands() + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
        }

        proc->setFinished(true);

        lock.lock();
        core->assignedProcess = nullptr;
        core->busy = false;
    }
}
