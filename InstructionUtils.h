#pragma once

#include "Instruction.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

inline std::string getRandomVarName() {
    static uint64_t counter = 0;
    return "v" + std::to_string(counter++);
}

inline std::vector<Instruction> generateRandomInstructions(int totalIns) {
    std::vector<Instruction> result;
    std::vector<std::string> declaredVars;

    for (int i = 0; i < totalIns; ++i) {
        Instruction instr;
        int type = rand() % 5;

        switch (type) {
            case 0: { // PRINT
                instr.type = InstructionType::PRINT;
                instr.message = "Hello world from process!";

                if (!declaredVars.empty()) {
                    instr.var1 = declaredVars[rand() % declaredVars.size()];
                }
                break;
            }

            case 1: { // DECLARE
                instr.type = InstructionType::DECLARE;
                instr.var1 = getRandomVarName();
                instr.value = rand() % 100;
                declaredVars.push_back(instr.var1);
                break;
            }

            case 2: { // ADD
                instr.type = InstructionType::ADD;
                instr.var1 = getRandomVarName();
                instr.var2 = declaredVars.empty() ? getRandomVarName() : declaredVars[rand() % declaredVars.size()];
                instr.var3 = declaredVars.empty() ? getRandomVarName() : declaredVars[rand() % declaredVars.size()];
                declaredVars.push_back(instr.var1);
                break;
            }

            case 3: { // SUBTRACT
                instr.type = InstructionType::SUBTRACT;
                instr.var1 = getRandomVarName();
                instr.var2 = declaredVars.empty() ? getRandomVarName() : declaredVars[rand() % declaredVars.size()];
                instr.var3 = declaredVars.empty() ? getRandomVarName() : declaredVars[rand() % declaredVars.size()];
                declaredVars.push_back(instr.var1);
                break;
            }

            case 4: { // SLEEP
                instr.type = InstructionType::SLEEP;
                instr.sleepTicks = rand() % 10 + 1;
                break;
            }
        }

        result.push_back(instr);
    }

    return result;
}
