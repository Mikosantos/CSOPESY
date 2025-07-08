#pragma once
#include "MemoryAllocator.h"
#include "ConsolePanel.h"
#include "Process.h"
#include <iomanip>
#include <sstream> 
#include <vector>
#include <memory>


struct MemoryPartition {
    uint32_t slotNo;

    bool isAllocatable;
    std::shared_ptr<Process> process;
};

class FlatMemoryAllocator : public MemoryAllocator{
    public:
        FlatMemoryAllocator(unsigned long long maxOverallMem, 
                            unsigned long long memPerFrame, 
                            unsigned long long memPerProc);

        bool allocate(std::shared_ptr<Process> process) override;
        void deallocate(std::shared_ptr<Process> process) override;
        void visualizeMemory() override;
        
        void displayMemoryPerCycle(uint32_t commandCounter);
        void setIsAllocatable(uint32_t slotNumber, bool value);
        
        uint32_t getAmountOfAllocatedSlots();
        uint32_t getFragmentation();

        bool hasFreeSlots();
        bool isProcessAllocated(std::shared_ptr<Process> process);

    private:
        std::vector<MemoryPartition> memoryPartitions;
        unsigned long long maxOverallMem;
        unsigned long long memPerFrame;
        unsigned long long memPerProc;
};
