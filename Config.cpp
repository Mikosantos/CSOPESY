#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Config loadConfig(const std::string& filePath) {  // ✅ No default here
    Config config;
    std::ifstream file(filePath);
    std::string key, value;

    if (!file.is_open()) {
        std::cerr << "Failed to open " << filePath << "\n";
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> key >> std::ws;
        std::getline(iss, value);

        // Remove quotes if any
        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());

        if (key == "num-cpu") config.numCPUs = std::stoi(value);
        else if (key == "scheduler") config.schedulerType = value;
        else if (key == "quantum-cycles") config.quantumCycles = std::stoull(value);
        else if (key == "batch-process-freq") config.batchProcessFreq = std::stoull(value);
        else if (key == "min-ins") config.minInstructions = std::stoull(value);
        else if (key == "max-ins") config.maxInstructions = std::stoull(value);
        else if (key == "delays-per-exec") config.delaysPerExec = std::stoull(value);

        // MO2 NEW FIELDS
        else if (key == "max-overall-mem") config.maxOverallMemory = std::stoull(value);
        else if (key == "mem-per-frame") config.memPerFrame = std::stoull(value);
        else if (key == "min-mem-per-proc") config.minMemPerProcess = std::stoull(value);
        else if (key == "max-mem-per-proc") config.maxMemPerProcess = std::stoull(value);
    }

    return config;
}
