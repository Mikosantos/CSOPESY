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

void Scheduler::start() {
    running = true;

    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&Scheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
}

void Scheduler::stop() {
    running = false;

    for (auto& core : cores) {
        core->cv.notify_all();
    }

    if (schedulerThread.joinable()) schedulerThread.join();

    for (auto& core : cores) {
        if (core->thread.joinable()) {
            core->thread.join();
        }
    }
}

void Scheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
}

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
