#include "FCFSScheduler.h"
#include <chrono>
#include <thread>

FCFSScheduler::FCFSScheduler(int cores, int delay) : Scheduler(cores, delay) {}

FCFSScheduler::~FCFSScheduler() {
    stop();
}

void FCFSScheduler::start() {
    running = true;

    for (int i = 0; i < coreCount; ++i) {
        auto core = std::make_unique<CPUCore>();
        core->thread = std::thread(&FCFSScheduler::coreWorker, this, i);
        cores.push_back(std::move(core));
    }

    schedulerThread = std::thread(&FCFSScheduler::schedulerLoop, this);

    tickThread = std::thread([this]() {
        while (running) {
            cpuTicks++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
}

void FCFSScheduler::stop() {
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

void FCFSScheduler::addProcess(const std::shared_ptr<Process>& proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(proc);
}

void FCFSScheduler::schedulerLoop() {
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

void FCFSScheduler::coreWorker(int coreId) {
    auto& core = cores[coreId];

    while (running) {
        std::unique_lock<std::mutex> lock(core->lock);
        core->cv.wait(lock, [&]() {
            return core->assignedProcess != nullptr || !running;
        });

        if (!running) break;

        auto proc = core->assignedProcess;
        lock.unlock();

        while (running && proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {
            if (proc->isSleeping(cpuTicks.load())) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            proc->executeInstruction(coreId, cpuTicks.load());

            int startTick = cpuTicks.load();
            while (running && (cpuTicks.load() - startTick < delayPerExec)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }

        proc->setFinished(true);
        lock.lock();
        core->assignedProcess = nullptr;
        core->busy = false;
        lock.unlock();
    }
}


// getters for printing system summary
// int Scheduler::getBusyCoreCount() const {
//     int count = 0;
//     for (const auto& core : cores) {
//         std::lock_guard<std::mutex> lock(core->lock);
//         if (core->busy) {
//             count++;
//         }
//     }
//     return count;
// }

// int Scheduler::getAvailableCoreCount() const {
//     return coreCount - getBusyCoreCount();
// }