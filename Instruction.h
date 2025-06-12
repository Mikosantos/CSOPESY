#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR
};

struct Instruction {
    InstructionType type;

    // For PRINT and message-based instructions
    std::string message; // default "Helo world from process_name!"

    // For variable operations (ADD, SUBTRACT)
    std::string var1, var2, var3;

    // For DECLARE
    uint16_t value = 0;

    // For SLEEP
    uint8_t sleepTicks = 0;

    // For FOR loop
    std::vector<Instruction> loopBody;
    int repeatCount = 0;
};
