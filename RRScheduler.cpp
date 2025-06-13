#include "RRScheduler.h"
#include <chrono>
#include <thread>

RRScheduler::RRScheduler(int cores, int delay, int quantum)
    : Scheduler(cores, delay), quantumCycles(quantum) {}

void RRScheduler::start() {
    running = true;

    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&RRScheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    schedulerThread = std::thread(&RRScheduler::schedulerLoop, this);

    tickThread = std::thread([this]() {
        while (running) {
            cpuTicks++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
}

void RRScheduler::stop() {
    running = false;

    for (auto& core : cores) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.notify_all();
    }

    if (schedulerThread.joinable()) schedulerThread.join();
    for (auto& core : cores) {
        if (core->thread.joinable()) core->thread.join();
    }
    if (tickThread.joinable()) tickThread.join();
}

void RRScheduler::addProcess(const std::shared_ptr<Process>& proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(proc);
}

void RRScheduler::schedulerLoop() {
    while (running) {
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
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void RRScheduler::coreWorker(int coreId) {
    auto& core = cores[coreId];

    while (running) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.wait(lock, [&]() {
            return core->assignedProcess != nullptr || !running;
        });

        if (!running) break;

        auto proc = core->assignedProcess;
        lock.unlock();

        int executedTicks = 0;

        while (running && executedTicks < quantumCycles) {
            if (proc->isFinished()) break;
            if (proc->isSleeping(cpuTicks.load())) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            proc->executeInstruction(coreId, cpuTicks.load());
            executedTicks++;

            int startTick = cpuTicks.load();
            while (running && (cpuTicks.load() - startTick < delayPerExec)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }

        // Process quantum finished or process is done
        if (proc->getCompletedCommands() >= proc->getTotalNoOfCommands()) {
            proc->setFinished(true);
        } else {
            std::lock_guard<std::mutex> qLock(queueMutex);
            readyQueue.push(proc);
        }


        lock.lock();
        core->assignedProcess = nullptr;
        core->busy = false;
        lock.unlock();
        proc->setCoreNum(-1);
    }
}

