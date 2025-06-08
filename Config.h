#pragma once
#include <string>

struct Config {
    int numCPUs;
    std::string schedulerType;
    int quantumCycles;
    int batchProcessFreq;
    int minInstructions;
    int maxInstructions;
    int delaysPerExec;
};

Config loadConfig(const std::string& filePath = "config.txt");
