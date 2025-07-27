#include "MemoryManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>

// TODO: CHECK IMPLEMENTATION OF MEMORY MANAGER (DEMAND PAGING, PAGE REPLACEMENT, ETC.)

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

void MemoryManager::allocateProcess(int pid, int memoryBytes) {
    int numPages = (memoryBytes + pageSize - 1) / pageSize;
    pageTables[pid] = std::vector<PageTableEntry>(numPages);
}

bool MemoryManager::ensurePageLoaded(int pid, int pageNo) {
    auto& pageTable = pageTables[pid];

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

void MemoryManager::writeByte(int pid, int virtualAddress, uint16_t value) {
    int pageNo = virtualAddress / pageSize;
    int offset = virtualAddress % pageSize;

    if (!ensurePageLoaded(pid, pageNo)) return;

    auto& pageTable = pageTables[pid];
    if (pageNo >= pageTable.size()) {
        std::cerr << "[ERROR] writeByte: Page number " << pageNo << " out of bounds for pid " << pid << "\n";
        return;
    }

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

uint16_t MemoryManager::readByte(int pid, int virtualAddress) {
    int pageNo = virtualAddress / pageSize;
    int offset = virtualAddress % pageSize;

    if (!ensurePageLoaded(pid, pageNo)) return 0;

    int frameNo = pageTables[pid][pageNo].frameNo;

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

void MemoryManager::savePageToBackingStore(int pid, int pageNo, int frameNo) {
    // TODO: FIX THIS
    // FORMAT SHOULD BE PROCESS_NAME IDK WHAT ELSE
    std::ofstream ofs("csopesy-backing-store.txt", std::ios::app);
    if (!ofs.is_open()) return;

    ofs << pid << ":" << pageNo << ":";
    for (auto byte : physicalMemory[frameNo].data) {
        ofs << static_cast<int>(byte) << " ";
    }
    ofs << "\n";
}

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

size_t MemoryManager::getUsedMemoryBytes() const {
    size_t usedFrames = numFrames - freeFrames.size();
    return usedFrames * pageSize;
}
