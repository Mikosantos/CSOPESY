#pragma once
#include <string>

struct Config {
    int numCPUs;
    std::string schedulerType;
    unsigned long long quantumCycles;
    unsigned long long batchProcessFreq;
    unsigned long long minInstructions;
    unsigned long long maxInstructions;
    unsigned long long delaysPerExec;
    size_t maxOverallMemory;
    size_t memPerFrame;
    size_t minMemPerProcess;
    size_t maxMemPerProcess;
};

Config loadConfig(const std::string& filePath = "config.txt");