#pragma once

#include "Instruction.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <iostream>

inline std::vector<Instruction> generateRandomInstructions(unsigned long long targetCount, const std::string& processName);

// Generate a random variable name
inline std::string getRandomVarName() {
    static uint64_t counter = 0;
    return "v" + std::to_string(counter++);
}

// Generate a timestamp in the format (MM/DD/YYYY HH:MM:SS AM/PM)
// indicates execution time of the instruction
inline std::string generateCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_time;
    localtime_s(&local_time, &now_c);

    std::ostringstream timestamp;
    timestamp << "("
              << std::setw(2) << std::setfill('0') << local_time.tm_mon + 1 << "/"
              << std::setw(2) << std::setfill('0') << local_time.tm_mday << "/"
              << (local_time.tm_year + 1900) << " ";

    int hour = local_time.tm_hour;
    std::string ampm = "AM";
    if (hour >= 12) {
        ampm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;

    timestamp << std::setw(2) << std::setfill('0') << hour << ":"
              << std::setw(2) << std::setfill('0') << local_time.tm_min << ":"
              << std::setw(2) << std::setfill('0') << local_time.tm_sec << " "
              << ampm << ")";

    return timestamp.str();
}

// for loop instruction generator
inline Instruction makeRandomForLoop(const std::string& processName, int depth = 0) {
    Instruction instr;
    instr.type = InstructionType::FOR;
    instr.loopRepeat = 1 + rand() % 3;

    int numInnerInstructions = 1 + rand() % 5;
    for (int i = 0; i < numInnerInstructions; ++i) {
        if (depth < 2 && rand() % 5 == 0) {
            instr.loopInstructions.push_back(makeRandomForLoop(processName, depth + 1));
        } else {
            instr.loopInstructions.push_back(generateRandomInstructions(1, processName)[0]);
        }
    }

    return instr;
}

// used to count the number of instructions inside a FOR loop
inline unsigned long long countInstructionsInFor(const Instruction& instr) {
    if (instr.type != InstructionType::FOR) return 0;

    unsigned long long count = 0;
    for (const auto& inner : instr.loopInstructions) {
        if (inner.type == InstructionType::FOR) {
            count += countInstructionsInFor(inner);
        } else {
            count++;
        }
    }

    return count * instr.loopRepeat;
}

// Generate random instructions for each running process; FOR SCHEDULER-START COMMAND
inline std::vector<Instruction> generateRandomInstructions(unsigned long long targetCount, const std::string& processName) {
    std::vector<Instruction> result;
    std::vector<std::string> declaredVars;
    unsigned long long actualCount = 0;

    while (actualCount < targetCount) {
        Instruction instr;
        int type = rand() % 8;

        if (declaredVars.empty()) {
            type = 1; // Force DECLARE until at least 1 variable exists
        }

        switch (type) {
            case 0: { // PRINT
                instr.type = InstructionType::PRINT;
                instr.message = "Hello from process " + processName + "!";
                if (!declaredVars.empty()) {
                    instr.var1 = declaredVars[rand() % declaredVars.size()];
                }
                result.push_back(instr);
                actualCount++;
                break;
            }

            case 1: { // DECLARE
                if (declaredVars.size() >= 32) continue; // symbol table full = skip forever
                instr.type = InstructionType::DECLARE;
                instr.var1 = getRandomVarName();
                instr.value = rand() % 65536; // max of uint_16
                declaredVars.push_back(instr.var1);
                result.push_back(instr);
                actualCount++;
                break;
            }

            case 2: { // ADD
                if (declaredVars.empty()) continue; // skip if no declared vars

                instr.type = InstructionType::ADD;
                instr.var1 = declaredVars[rand() % declaredVars.size()];  // result target must be existing

                // Operand 2
                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var2 = declaredVars[rand() % declaredVars.size()];
                    instr.var2IsImmediate = false;
                } else {
                    instr.var2IsImmediate = true;
                    instr.var2ImmediateValue = rand() % 65536;
                }

                // Operand 3
                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var3 = declaredVars[rand() % declaredVars.size()];
                    instr.var3IsImmediate = false;
                } else {
                    instr.var3IsImmediate = true;
                    instr.var3ImmediateValue = rand() % 65536;
                }

                result.push_back(instr);
                actualCount++;
                break;
            }

            case 3: { // SUBTRACT
                if (declaredVars.empty()) continue; // skip if no declared vars

                instr.type = InstructionType::SUBTRACT;
                instr.var1 = declaredVars[rand() % declaredVars.size()];  // use existing var

                // Operand 2
                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var2 = declaredVars[rand() % declaredVars.size()];
                    instr.var2IsImmediate = false;
                } else {
                    instr.var2IsImmediate = true;
                    instr.var2ImmediateValue = rand() % 65536;
                }

                // Operand 3
                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var3 = declaredVars[rand() % declaredVars.size()];
                    instr.var3IsImmediate = false;
                } else {
                    instr.var3IsImmediate = true;
                    instr.var3ImmediateValue = rand() % 65536;
                }

                result.push_back(instr);
                actualCount++;
                break;
            }

            case 4: { // SLEEP
                instr.type = InstructionType::SLEEP;
                instr.sleepTicks = rand() % 10 + 1;
                result.push_back(instr);
                actualCount++;
                break;
            }

            case 5: { // FOR
                instr = makeRandomForLoop(processName);

                // only for counting the instructions inside the FOR loop
                int count = countInstructionsInFor(instr);

                if (actualCount + count <= targetCount) {
                    result.push_back(instr);
                    actualCount += count;
                }
                // else skip this FOR
                break;
            }

            case 6: { // READ
                if (declaredVars.empty()) continue; // skip if no declared vars

                instr.type = InstructionType::READ;
                instr.var1 = declaredVars[rand() % declaredVars.size()];
                
                // Generate a valid hex memory address (within process memory range, e.g., 0x0000–0x3FFF)
                std::stringstream ss;
                ss << "0x" << std::hex << (rand() % 0x4000);  // adjust range as needed
                instr.memoryAddress = ss.str();

                result.push_back(instr);
                actualCount++;
                break;
            }

            case 7: { // WRITE
                if (declaredVars.empty()) continue; // skip if no declared vars

                instr.type = InstructionType::WRITE;

                // Generate hex memory address
                std::stringstream ss;
                ss << "0x" << std::hex << (rand() % 0x4000);
                instr.memoryAddress = ss.str();

                // Use existing declared variable
                instr.var1 = declaredVars[rand() % declaredVars.size()];

                result.push_back(instr);
                actualCount++;
                break;
            }
        }
    }

    return result;
}

// NEW MO2 INSTRUCTION GENERATOR; FOR SCREEN -C COMMAND
// This function generates fixed instructions based on a vector of raw instruction strings.

// TODO: FOR LOOP
inline std::vector<Instruction> generateFixedInstructions(const std::vector<std::string>& rawInstructions) {
    std::vector<Instruction> result;

    for (const auto& instrLine : rawInstructions) {
        std::istringstream iss(instrLine);
        std::string token;
        iss >> token;

        Instruction instr;

        if (token == "PRINT") {
            instr.type = InstructionType::PRINT;

            std::string restOfLine;
            getline(iss, restOfLine);

            // Remove parentheses and quotes if present
            size_t start = restOfLine.find_first_of("\"(");
            size_t end = restOfLine.find_last_of("\")");
            if (start != std::string::npos && end != std::string::npos && end > start) {
                instr.message = restOfLine.substr(start + 1, end - start - 1);
            } else {
                instr.message = restOfLine; // fallback
            }
        }

        else if (token == "DECLARE") {
            instr.type = InstructionType::DECLARE;
            iss >> instr.var1 >> instr.value;

            // std::cout << "instruction: " << instrLine << "\n"; //DEBUGGING
        }

        else if (token == "ADD") {
            instr.type = InstructionType::ADD;
            iss >> instr.var1 >> instr.var2 >> instr.var3;

            instr.var2IsImmediate = isdigit(instr.var2[0]);
            instr.var3IsImmediate = isdigit(instr.var3[0]);

            if (instr.var2IsImmediate) instr.var2ImmediateValue = std::stoi(instr.var2);
            if (instr.var3IsImmediate) instr.var3ImmediateValue = std::stoi(instr.var3);

            // std::cout << "instruction: " << instrLine << "\n"; //DEBUGGING
        }

        else if (token == "SUBTRACT") {
            instr.type = InstructionType::SUBTRACT;
            iss >> instr.var1 >> instr.var2 >> instr.var3;

            instr.var2IsImmediate = isdigit(instr.var2[0]);
            instr.var3IsImmediate = isdigit(instr.var3[0]);

            if (instr.var2IsImmediate) instr.var2ImmediateValue = std::stoi(instr.var2);
            if (instr.var3IsImmediate) instr.var3ImmediateValue = std::stoi(instr.var3);
        }

        else if (token == "SLEEP") {
            instr.type = InstructionType::SLEEP;
            iss >> instr.sleepTicks;
        }

        else if (token == "READ") {
            instr.type = InstructionType::READ;
            iss >> instr.var1 >> instr.memoryAddress;
        }

        else if (token == "WRITE") {
            instr.type = InstructionType::WRITE;
            iss >> instr.memoryAddress >> instr.var1;
        }

        else {
            std::cout << "Unknown instruction: " << token << "\n";
            continue; // skip invalid
        }

        result.push_back(instr);
    }

    return result;
}


/*
    This function counts the total number of instructions in a vector of instructions,
    including those expanded from FOR loops.
*/
inline int countExpandedInstructions(const std::vector<Instruction>& instructions) {
    int count = 0;

    for (const auto& instr : instructions) {
        if (instr.type == InstructionType::FOR) {
            count += countExpandedInstructions(instr.loopInstructions) * instr.loopRepeat;
        } else {
            count++;
        }
    }

    return count;
}
