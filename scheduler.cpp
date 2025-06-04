#include "Scheduler.h"
#include <iostream>
#include <chrono>
#include <fstream>

Scheduler::Scheduler(int cores) : running(false), coreCount(cores) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    running = true;
    for (int i = 0; i < coreCount; ++i) {
        cpuCores.emplace_back(&Scheduler::runCore, this, i);
    }
}

void Scheduler::stop() {
    running = false;
    cv.notify_all();
    for (auto& thread : cpuCores) {
        if (thread.joinable()) thread.join();
    }
}

void Scheduler::addProcess(const std::shared_ptr<Process>& proc) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(proc);
    }
    cv.notify_one();
}

void Scheduler::runCore(int coreId) {
    while (running) {
        std::shared_ptr<Process> proc = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() {
                return !readyQueue.empty() || !running;
            });

            if (!running) break;

            proc = readyQueue.front();
            readyQueue.pop();
        }

        proc->setCoreNum(coreId);
        proc->setFinished(false);

        while (proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {
            // Simulate execution of a PRINT instruction
            std::ofstream file(proc->getProcessName() + ".txt", std::ios::app);
            file << proc->getTime() << " Core:" << coreId
                 << " \"Hello world from " << proc->getProcessName() << "!\"\n";

            proc->setCompletedCommands(proc->getCompletedCommands() + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // simulate tick
        }

        proc->setFinished(true);
    }
}
