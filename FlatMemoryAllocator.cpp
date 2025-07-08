#include "FlatMemoryAllocator.h"
#include <algorithm>
#include <filesystem>


/**
 * @brief Constructor for FlatMemoryAllocator.
 * @param maxOverallMem Maximum memory size for the system.
 * @param memPerFrame Memory size of each frame.
 * @param memPerProc Memory required per process.
 */

FlatMemoryAllocator::FlatMemoryAllocator(unsigned long long maxOverallMem, unsigned long long memPerFrame, unsigned long long memPerProc): 
                    MemoryAllocator(), maxOverallMem(maxOverallMem), memPerFrame(memPerFrame), memPerProc(memPerProc){
    // Define partition size based on frame size.
    uint32_t numPartitions = maxOverallMem / memPerProc;
    for (uint32_t i = 0; i < numPartitions; ++i) {
        memoryPartitions.push_back({ i, true, nullptr });   // Create 4 empty memory partitions.
    }
};

/** The first-fit memory allocation strategy looks for the first available memory slot 
 *  (partition) that can hold a process and allocates it there. It doesn’t search the 
 *  entire list — just enough to find the first free one.
 *
 * @brief Attempts to allocate memory for a process.
 * @param process Shared pointer to the process to allocate.
 * @return true if successful, false if already allocated or no slot available.
 */
bool FlatMemoryAllocator::allocate(std::shared_ptr<Process> process)
{   // Step 1: Check if the process is already allocated to avoid duplicates.
    for (const auto& partition : memoryPartitions) {
        if (partition.process == process) {
            return false;  // Process already exists in memory.
        }
    }
     // Step 2: Find the first free (allocatable) memory partition.
    for (auto& partition : memoryPartitions) {
        if (partition.isAllocatable) {
             // First-fit match found, assign the process.
            partition.process = process;        // Assign process to partition
            partition.isAllocatable = false;    // Mark partition as occupied
            return true;                        // Allocation successful
        }
    }

    return false;                               // No free partitions
}

/**
 * @brief Frees memory associated with a process.
 * @param process Shared pointer to the process to deallocate.
 */

void FlatMemoryAllocator::deallocate(std::shared_ptr<Process> process)
{
    for (auto& partition : memoryPartitions) {
        if (partition.process == process) {
            partition.process.reset();          // Remove the process from memory
            partition.isAllocatable = true;     // Mark slot as available
        }
    }
}

/**
 * @brief Prints the current memory layout with state.
 */
void FlatMemoryAllocator::visualizeMemory(){
    // Starting memory
    uint32_t totalMemory = maxOverallMem;  
    // Each partition slot size     
    uint32_t slotSize = memPerProc;
    // Buffer to build the output
    std::ostringstream outputBuffer;  
    uint32_t currentMemory = totalMemory;

    // Count of used slots.
    uint32_t numProcessesInMemory = getAmountOfAllocatedSlots();
    uint32_t totalExternalFragmentation = 0;

    // Get current time
    time_t now = time(0);
    struct tm localTime;
    localtime_s(&localTime, &now);
    outputBuffer << "\nTimestamp: (" << std::put_time(&localTime, "%m/%d/%Y %I:%M:%S%p") << ")\n";

    outputBuffer << "Number of processes in memory: " << numProcessesInMemory << "\n";
    outputBuffer << "Total external fragmentation in KB: " << getFragmentation() << "\n";

    outputBuffer << "----end---- = " << totalMemory << "\n";

    for (size_t i = 0; i < memoryPartitions.size(); ++i) {
        const auto& partition = memoryPartitions[i];
        if (partition.isAllocatable) {
            outputBuffer << "\n";
            totalMemory -= slotSize;
        }
        else if (partition.process) {
            std::string processName = partition.process->getProcessName();


            if (partition.process->getProcessState() == ProcessState::WAITING) { 
                processName += " *";
            }

            outputBuffer << processName << "\n";
            totalMemory -= slotSize;  
            numProcessesInMemory++; 
        }

        if (i < memoryPartitions.size() - 1) {
            outputBuffer << totalMemory << "\n";
        }
    }

    totalExternalFragmentation = totalMemory;
    outputBuffer << "----start---- = 0\n\n";
    std::cout << outputBuffer.str();
}   

/**
 * @brief Writes memory layout to a file for each cycle.
 * @param commandCounter Counter used to generate unique filenames.
 */
void FlatMemoryAllocator::displayMemoryPerCycle(uint32_t commandCounter)
{
    uint32_t totalMemory = maxOverallMem;
    uint32_t slotSize = memPerProc;

    std::ostringstream buffer;

    time_t now = time(0);
    struct tm localTime;
    localtime_s(&localTime, &now);

    // Header
    buffer << "\nTimestamp: (" << std::put_time(&localTime, "%m/%d/%Y %I:%M:%S%p") << ")\n";
    buffer << "Number of processes in memory: " << getAmountOfAllocatedSlots() << "\n";
    buffer << "Total external fragmentation in KB: " << getFragmentation() << "\n";
    buffer << "----end---- = " << totalMemory << "\n";

    uint32_t currentTop = totalMemory;

     for (const auto& partition : memoryPartitions) {
        uint32_t upperLimit = currentTop;
        uint32_t lowerLimit = currentTop - slotSize;

        if (partition.process) {
            buffer << upperLimit << "\n";                        // Only print range for occupied
            buffer << partition.process->getProcessName() << "\n";
            buffer << lowerLimit << "\n\n";
        }

        currentTop -= slotSize;
    }

    buffer << "----start---- = 0\n";

    // File output
    std::ostringstream filename;
    filename << "memory_stamp_" << commandCounter << ".txt";
    std::filesystem::path logDirectory = std::filesystem::current_path() / "Memory_Logs";
    std::filesystem::create_directories(logDirectory);
    std::filesystem::path filePath = logDirectory / filename.str();

    std::ofstream logFile(filePath, std::ios::trunc);
    if (!logFile.is_open()) {
        std::cerr << "Error: Unable to open memory log file.\n";
        return;
    }

    logFile << buffer.str();
    logFile.close();
}


/**
 * @brief Calculates external fragmentation in memory.
 * @return External fragmentation in KB.
 */
uint32_t FlatMemoryAllocator::getFragmentation() {
    uint32_t frag = 0;
    uint32_t rangeStart = 0;
    uint32_t rangeEnd = memPerProc;

    // Iterate through each partition (each of size memPerProc)
    for (const auto& part : memoryPartitions) {
        if (!part.process) {
            // If no process in this partition, it's external fragmentation
            frag += (rangeEnd - rangeStart);
        }
        // Move to next memory range
        rangeStart += memPerProc;
        rangeEnd += memPerProc;
    }

    return frag; // Convert bytes to KB
}



/**
 * @brief Checks if a process is currently in memory.
 * @param process The process to look for.
 * @return true if found, false otherwise.
 */
bool FlatMemoryAllocator::isProcessAllocated(std::shared_ptr<Process> process){
    for (const auto& partition : memoryPartitions) {
        if (partition.process == process) {
            return true;    // Match found
        }
    }
    return false; 
}

/**
 * @brief Sets the allocation status of a memory slot.
 * @param slotNumber Index of the slot.
 * @param value true = free, false = occupied.
 */
void FlatMemoryAllocator::setIsAllocatable(uint32_t slotNumber, bool value){
    memoryPartitions[slotNumber].isAllocatable = value;
}

/**
 * @brief Gets the number of currently allocated memory slots.
 * @return Count of used slots.
 */
uint32_t FlatMemoryAllocator::getAmountOfAllocatedSlots(){
    uint32_t allocatedSlots = 0;
    for (const auto& partition : memoryPartitions) {
        if (!partition.isAllocatable) {
            allocatedSlots++;       // Count used partition
        }
    }
    return allocatedSlots;
}

/**
 * @brief Checks whether there is at least one free memory slot.
 * @return true if any slot is free, false otherwise.
 */

bool FlatMemoryAllocator::hasFreeSlots(){
    for (const auto& partition : memoryPartitions) {
        if (partition.isAllocatable) {
            return true;
        }
    }
    return false;
}

