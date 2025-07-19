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
    unsigned long long maxOverallMemory;
    unsigned long long memPerFrame;
    unsigned long long minMemPerProcess;
    unsigned long long maxMemPerProcess;
};

Config loadConfig(const std::string& filePath = "config.txt");