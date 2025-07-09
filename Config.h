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
    unsigned long long maxOverallMem;
    unsigned long long memPerFrame;
    unsigned long long memPerProc;
};

Config loadConfig(const std::string& filePath = "config.txt");
