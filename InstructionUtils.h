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

std::vector<Instruction> generateRandomInstructions(int totalIns);

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
inline Instruction makeRandomForLoop(int depth = 0) {
    Instruction instr;
    instr.type = InstructionType::FOR;
    instr.loopRepeat = 1 + rand() % 3;

    int numInnerInstructions = 1 + rand() % 5;
    for (int i = 0; i < numInnerInstructions; ++i) {
        if (depth < 2 && rand() % 5 == 0) {
            instr.loopInstructions.push_back(makeRandomForLoop(depth + 1));
        } else {
            instr.loopInstructions.push_back(generateRandomInstructions(1)[0]);
        }
    }

    return instr;
}

inline int countInstructionsInFor(const Instruction& instr) {
    if (instr.type != InstructionType::FOR) return 0;

    int count = 0;
    for (const auto& inner : instr.loopInstructions) {
        if (inner.type == InstructionType::FOR) {
            count += countInstructionsInFor(inner);
        } else {
            count++;
        }
    }

    return count * instr.loopRepeat;
}

// Generate random instructions for each running process
inline std::vector<Instruction> generateRandomInstructions(int targetCount) {
    std::vector<Instruction> result;
    std::vector<std::string> declaredVars;
    int actualCount = 0;

    while (actualCount < targetCount) {
        Instruction instr;
        int type = rand() % 6;

        switch (type) {
            case 0: { // PRINT
                instr.type = InstructionType::PRINT;
                instr.message = "Hello world from process!";
                if (!declaredVars.empty()) {
                    instr.var1 = declaredVars[rand() % declaredVars.size()];
                }
                result.push_back(instr);
                actualCount++;
                break;
            }

            case 1: { // DECLARE
                instr.type = InstructionType::DECLARE;
                instr.var1 = getRandomVarName();
                instr.value = rand() % 100;
                declaredVars.push_back(instr.var1);
                result.push_back(instr);
                actualCount++;
                break;
            }

            case 2: { // ADD
                instr.type = InstructionType::ADD;
                instr.var1 = getRandomVarName();

                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var2 = declaredVars[rand() % declaredVars.size()];
                    instr.var2IsImmediate = false;
                } else {
                    instr.var2IsImmediate = true;
                    instr.var2ImmediateValue = rand() % 100;
                }

                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var3 = declaredVars[rand() % declaredVars.size()];
                    instr.var3IsImmediate = false;
                } else {
                    instr.var3IsImmediate = true;
                    instr.var3ImmediateValue = rand() % 100;
                }

                declaredVars.push_back(instr.var1);
                result.push_back(instr);
                actualCount++;
                break;
            }

            case 3: { // SUBTRACT
                instr.type = InstructionType::SUBTRACT;
                instr.var1 = getRandomVarName();

                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var2 = declaredVars[rand() % declaredVars.size()];
                    instr.var2IsImmediate = false;
                } else {
                    instr.var2IsImmediate = true;
                    instr.var2ImmediateValue = rand() % 100;
                }

                if (!declaredVars.empty() && rand() % 2 == 0) {
                    instr.var3 = declaredVars[rand() % declaredVars.size()];
                    instr.var3IsImmediate = false;
                } else {
                    instr.var3IsImmediate = true;
                    instr.var3ImmediateValue = rand() % 100;
                }

                declaredVars.push_back(instr.var1);
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
                instr = makeRandomForLoop();

                // only for counting the instructions inside the FOR loop
                int count = countInstructionsInFor(instr);

                if (actualCount + count <= targetCount) {
                    result.push_back(instr);
                    actualCount += count;
                }
                // else skip this FOR to avoid overshooting
                break;
            }
        }
    }

    return result;
}

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
