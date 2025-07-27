// MemoryManager.h
#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <string>
#include <fstream>
#include <cstdint>

struct Frame {
    int processId = -1;
    int virtualPageNo = -1;
    bool dirty = false;
    std::vector<uint8_t> data; // PAGE SIZE allocated dynamically

    Frame() : data() {} // default constructor
};

struct PageTableEntry {
    int frameNo = -1;
    bool valid = false;
    bool dirty = false;
};

class MemoryManager {
private:
    size_t totalMemory;
    size_t pageSize;
    int numFrames;

    std::vector<Frame> physicalMemory;
    std::unordered_map<int, std::vector<PageTableEntry>> pageTables;
    std::queue<int> freeFrames;
    std::queue<std::pair<int, int>> fifoQueue;

    int pagesPagedIn = 0;
    int pagesPagedOut = 0;

public:
    MemoryManager(size_t totalMemoryBytes, size_t memoryPerFrame);

    void allocateProcess(int pid, int memoryBytes);
    uint16_t readByte(int pid, int virtualAddress);
    void writeByte(int pid, int virtualAddress, uint16_t value);

    bool ensurePageLoaded(int pid, int vpn);

    void savePageToBackingStore(int pid, int vpn, int frameNo);
    void loadPageFromBackingStore(int pid, int vpn, int frameNo);

    // Getters
    int getPagesPagedIn() const { return pagesPagedIn; }
    int getPagesPagedOut() const { return pagesPagedOut; }
    int getFreeFrameCount() const { return freeFrames.size(); }
    int getUsedFrameCount() const { return numFrames - freeFrames.size(); }
    size_t getTotalMemory() const { return totalMemory; }
    size_t getPageSize() const { return pageSize; }

    size_t getUsedMemoryBytes() const;

    const std::vector<Frame>& getFrames() const { return physicalMemory; }
};