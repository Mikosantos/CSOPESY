#include "MemoryManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>

// TODO: CHECK IMPLEMENTATION OF MEMORY MANAGER (DEMAND PAGING, PAGE REPLACEMENT, ETC.)

/*
    * MemoryManager constructor initializes the memory manager with the total memory size and page size.
    * It calculates the number of frames and initializes the physical memory and free frames.
    * It also initializes the backing store file.
*/
MemoryManager::MemoryManager(size_t totalMemory, size_t pageSize)
    : totalMemory(totalMemory), pageSize(pageSize), pagesPagedIn(0), pagesPagedOut(0) {

    numFrames = totalMemory / pageSize;
    physicalMemory.resize(numFrames);

    for (int i = 0; i < numFrames; ++i) {
        physicalMemory[i].data.resize(pageSize);
        freeFrames.push(i);
    }

    std::ofstream ofs("csopesy-backing-store.txt", std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "[ERROR] Failed to initialize backing store.\n";
    }
}

/*
    * allocateProcess allocates memory for a process by creating a page table for the given process ID.
    * It calculates the number of pages needed based on the memory size and page size.
*/
bool MemoryManager::allocateProcess(int pid, int memoryBytes) {
    std::lock_guard<std::mutex> lock(memoryMutex);

    int numPages = (memoryBytes + pageSize - 1) / pageSize;

    if (numPages > freeFrames.size()) {
        return false; // not enough memory
    }

    pageTables[pid] = std::vector<PageTableEntry>(numPages);
    return true;
}

/*
    * ensurePageLoaded checks if a page is loaded in memory for the given process ID and virtual page number.
    * If the page is not loaded, it attempts to load it from the backing store or evicts a page if necessary.
    * Returns true if the page is successfully loaded, false otherwise.
*/
bool MemoryManager::ensurePageLoaded(int pid, int pageNo) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    auto it = pageTables.find(pid);
    if (it == pageTables.end()) {
        // std::cerr << "[ERROR] ensurePageLoaded: No page table for PID " << pid << "\n";
        return false;
    }
    auto& pageTable = it->second;

    if (pageNo >= pageTable.size()) {
        // std::cerr << "[ERROR] Invalid page number for process " << pid << "\n";
        return false;
    }

    if (!pageTable[pageNo].valid) {
        int frameNo;

        if (!freeFrames.empty()) {
            frameNo = freeFrames.front();
            freeFrames.pop();
        } else if (!fifoQueue.empty()) {
            // Page replacement using FIFO
            auto victim = fifoQueue.front();
            fifoQueue.pop();

            int victimPid = victim.first;
            int victimPage = victim.second;
            auto& victimEntry = pageTables[victimPid][victimPage];

            frameNo = victimEntry.frameNo;
            if (victimEntry.dirty) {
                savePageToBackingStore(victimPid, victimPage, victimEntry.frameNo);
            }

            pageTables[victimPid][victimPage] = PageTableEntry{};
            ++pagesPagedOut;
        } else {
            return false; // No frames available and no FIFO queue to evict
        }

        loadPageFromBackingStore(pid, pageNo, frameNo);

        pageTable[pageNo].frameNo = frameNo;
        pageTable[pageNo].valid = true;
        pageTable[pageNo].dirty = false;
        ++pagesPagedIn;

        fifoQueue.emplace(pid, pageNo);
    }
    return true;
}

/*
    this function writes a byte value to the specified virtual address of the process.
    It first checks if the page is loaded, and if not, it loads the page.
    Then it writes the value to the specified offset in the frame corresponding to the page.
*/
void MemoryManager::writeByte(int pid, int virtualAddress, uint16_t value) {
    int pageNo = virtualAddress / pageSize;
    int offset = virtualAddress % pageSize;

    if (!ensurePageLoaded(pid, pageNo)) return;

    auto it = pageTables.find(pid);
    if (it == pageTables.end()) {
        std::cerr << "[ERROR] ensurePageLoaded: No page table for PID " << pid << "\n";
        return;
    }
    auto& pageTable = it->second;


    int frameNo = pageTable[pageNo].frameNo;

    if (frameNo < 0 || frameNo >= physicalMemory.size()) {
        std::cerr << "[ERROR] writeByte: Invalid frame number " << frameNo << " for page " << pageNo << "\n";
        return;
    }

    if (offset + 1 >= pageSize) {
        std::cerr << "[ERROR] writeByte: Not enough space to write 2 bytes at offset " << offset << "\n";
        return;
    }

    physicalMemory[frameNo].data[offset] = value & 0xFF;            // lower byte
    physicalMemory[frameNo].data[offset + 1] = (value >> 8) & 0xFF; // upper byte
    pageTable[pageNo].dirty = true;
}

/*
    this function reads a 2-byte value from the specified virtual address of the process.
    It first checks if the page is loaded, and if not, it loads the page.
    Then it reads the value from the specified offset in the frame corresponding to the page.
*/
uint16_t MemoryManager::readByte(int pid, int virtualAddress) {
    int pageNo = virtualAddress / pageSize;
    int offset = virtualAddress % pageSize;

    if (!ensurePageLoaded(pid, pageNo)) return 0;

    // int frameNo = pageTables[pid][pageNo].frameNo;
    auto it = pageTables.find(pid);
    if (it == pageTables.end()) {
        std::cerr << "[ERROR] readByte: No page table for PID " << pid << "\n";
        return 0;
    }
    auto& pageTable = it->second;

    if (pageNo >= pageTable.size()) {
        std::cerr << "[ERROR] readByte: Page number out of bounds for PID " << pid << "\n";
        return 0;
    }

    int frameNo = pageTable[pageNo].frameNo;

    if (frameNo < 0 || frameNo >= physicalMemory.size()) {
        std::cerr << "[ERROR] readByte: Invalid frame number " << frameNo << " for page " << pageNo << "\n";
        return 0;
    }

    if (offset + 1 >= pageSize) {
        std::cerr << "[ERROR] readByte: Not enough bytes to read 2-byte value at offset " << offset << "\n";
        return 0;
    }

    uint8_t low = physicalMemory[frameNo].data[offset];
    uint8_t high = physicalMemory[frameNo].data[offset + 1];
    return static_cast<uint16_t>(low | (high << 8));
}

/*
    this function saves a page to the backing store file.
    It writes the process ID, virtual page number, and the data of the page to the file.
    If the file cannot be opened, it prints an error message.

    kind of weird because the content is like this:
    process_id:virtual_page_no:byte1 byte2 byte3 ...
    where each byte is an integer value (0-255) representing the byte data of the page. and its too loong ??

    idk if its required to save the whole page data like this, but this is how it was implemented in the original code.
*/
void MemoryManager::savePageToBackingStore(int pid, int pageNo, int frameNo) {
    // TODO: FIX THIS
    std::ofstream ofs("csopesy-backing-store.txt", std::ios::app);
    if (!ofs.is_open()) return;

    ofs << pid << ":" << pageNo << ":";
    for (auto byte : physicalMemory[frameNo].data) {
        ofs << static_cast<int>(byte) << " ";
    }
    ofs << "\n";
}

/*
    this function loads a page from the backing store file.
    It reads the process ID, virtual page number, and the data of the page from the file.
    If the page is found, it loads the data into the specified frame in physical memory.
    If not found, it zeroes out the frame.
*/
void MemoryManager::loadPageFromBackingStore(int pid, int pageNo, int frameNo) {
    std::ifstream ifs("csopesy-backing-store.txt");
    std::string line;

    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string token;
        std::getline(iss, token, ':');
        int storedPid = std::stoi(token);
        std::getline(iss, token, ':');
        int storedPage = std::stoi(token);

        if (storedPid == pid && storedPage == pageNo) {
            for (int i = 0; i < pageSize && iss; ++i) {
                int val;
                iss >> val;
                physicalMemory[frameNo].data[i] = static_cast<uint8_t>(val);
            }
            return;
        }
    }

    // If not found in backing store, zero out the frame
    std::fill(physicalMemory[frameNo].data.begin(), physicalMemory[frameNo].data.end(), 0);
}

/*
    * getUsedMemoryBytes returns the total number of bytes used in memory.
    * It calculates the used memory by multiplying the number of used frames by the page size.
*/
size_t MemoryManager::getUsedMemoryBytes() const {
    size_t usedFrames = numFrames - freeFrames.size();
    return usedFrames * pageSize;
}

/*
    previous version of deallocateProcess
*/

void MemoryManager::deallocateProcess(int pid) {
    auto it = pageTables.find(pid);
    if (it == pageTables.end()) return;

    // Return used frames to free pool
    for (auto& entry : it->second) {
        if (entry.valid && entry.frameNo >= 0) {
            freeFrames.push(entry.frameNo);
        }
    }

    // Remove from FIFO queue
    std::queue<std::pair<int, int>> newQueue;
    while (!fifoQueue.empty()) {
        auto front = fifoQueue.front();
        fifoQueue.pop();
        if (front.first != pid) {
            newQueue.push(front);
        }
    }
    fifoQueue = std::move(newQueue);

    // Remove page table
    pageTables.erase(it);
}

/*
    * cleanBackingStore removes all entries for the given process ID from the backing store file.
    * It reads the file, filters out the entries for the specified PID, and writes the remaining entries back to the file.
*/
void MemoryManager::cleanBackingStore(int pid) {
    std::ifstream in("csopesy-backing-store.txt");
    if (!in.is_open()) return;

    std::ostringstream temp;
    std::string line;

    // Keep lines that are NOT for the given pid
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string pidStr;
        if (std::getline(iss, pidStr, ':')) {
            try {
                int storedPid = std::stoi(pidStr);
                if (storedPid != pid) {
                    temp << line << "\n";
                }
            } catch (...) {
                // If malformed line, keep it to avoid accidental data loss
                temp << line << "\n";
            }
        }
    }
    in.close();

    // Overwrite file with filtered content
    std::ofstream out("csopesy-backing-store.txt", std::ios::trunc);
    if (!out.is_open()) return;

    out << temp.str();
    out.close();
}

/*
    * deallocateProcess deallocates the memory for a process by removing its page table and freeing its frames.
    * It also cleans up the backing store entries for the process.
*/
// void MemoryManager::deallocateProcess(int pid) {
//     auto it = pageTables.find(pid);
//     if (it == pageTables.end()) return;

//     // Return used frames to free pool and clear memory
//     for (auto& entry : it->second) {
//         if (entry.valid && entry.frameNo >= 0 && entry.frameNo < physicalMemory.size()) {
//             std::fill(physicalMemory[entry.frameNo].data.begin(),
//                       physicalMemory[entry.frameNo].data.end(), 0);
//             freeFrames.push(entry.frameNo);
//         }
//     }

//     // Remove from FIFO queue
//     std::queue<std::pair<int, int>> newQueue;
//     while (!fifoQueue.empty()) {
//         auto front = fifoQueue.front();
//         fifoQueue.pop();
//         if (front.first != pid) {
//             newQueue.push(front);
//         }
//     }
//     fifoQueue = std::move(newQueue);

//     // Remove page table
//     pageTables.erase(it);

//     // Clean up backing store entries for this process
//     cleanBackingStore(pid);
// }