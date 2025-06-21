#include "Scheduler.h"

Scheduler::Scheduler(int cores, int delay)
    : coreCount(cores), delayPerExec(delay),
      coreTicks(cores) // directly initialize vector with cores default-constructed atomics
{
    for (int i = 0; i < coreCount; ++i)
        coreTicks[i].store(0);  // explicitly store 0 if needed
}

Scheduler::~Scheduler() {}

void Scheduler::addProcess(const std::shared_ptr<Process>& proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(proc);
}

// Used to know how many cores are currently busy
// A core is considered busy if it has an assigned process that is not finished
// This function iterates through all cores and checks if they are busy
// It locks each core's mutex to ensure thread safety while checking the busy status
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

