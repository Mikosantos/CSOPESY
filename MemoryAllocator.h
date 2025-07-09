#pragma once
#include <memory>
#include "Process.h"

class MemoryAllocator {
public:
    MemoryAllocator();
    virtual bool allocate(std::shared_ptr<Process> process) = 0;
    virtual void deallocate(std::shared_ptr<Process> process) = 0;
    virtual void visualizeMemory() = 0;
    virtual ~MemoryAllocator() = default;  
};
