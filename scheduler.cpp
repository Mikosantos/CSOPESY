#include "Scheduler.h"

Scheduler::Scheduler(int cores, unsigned long long delay)
    : coreCount(cores), delayPerExec(delay),
      coreTicks(cores) // directly initialize vector with cores default-constructed atomics
{
    for (int i = 0; i < coreCount; ++i)
        coreTicks[i].store(0);
}

Scheduler::~Scheduler() {}

void Scheduler::addProcess(const std::shared_ptr<Process>& proc) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(proc);
}

/*
    Used to know how many cores are currently busy
    A core is considered busy if it has an assigned process that is not finished
    This function iterates through all cores and checks if they are busy
    It locks each core's mutex to ensure thread safety while checking the busy status
*/
int Scheduler::getBusyCoreCount() const {
    int count = 0;
    for (const auto& core : cores) {
        std::lock_guard<std::mutex> lock(core->lock);
        if (core->assignedProcess != nullptr) { // edited
            count++;
        }
    }
    return count;
}

int Scheduler::getAvailableCoreCount() const {
    return coreCount - getBusyCoreCount();
}

std::vector<std::shared_ptr<Process>> Scheduler::getRunningProcesses() const {
    
    std::vector<std::shared_ptr<Process>> result;

    for (int i = 0; i < cores.size(); ++i) {
        std::shared_ptr<Process> proc;
        {
            std::lock_guard<std::mutex> lock(cores[i]->lock);
            proc = cores[i]->assignedProcess;

            if (proc && proc->getCompletedCommands() < proc->getTotalNoOfCommands()) {
                result.push_back(proc);
            }
        }
    }

    return result;
}