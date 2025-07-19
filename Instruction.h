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
    FOR,

    // NEW MO2 INSTRUCTION TYPES
    READ,    
    WRITE    
};

struct Instruction {
    InstructionType type;
    bool hasExecuted = false;
    int executedCore = -1;
    std::string executedTimestamp;

    // For PRINT
    std::string message; // default "Helo world from process_name!"

    // For variable operations (ADD, SUBTRACT)
    std::string var1, var2, var3;

     // Optional values (if var2/var3 is an immediate number/random number)
    bool var2IsImmediate = false;
    bool var3IsImmediate = false;
    uint16_t var2ImmediateValue = 0;
    uint16_t var3ImmediateValue = 0;

    // For DECLARE
    uint16_t value = 0;

    // For SLEEP
    uint8_t sleepTicks = 0;

    // For FOR loop
    std::vector<Instruction> loopInstructions;
    int loopRepeat = 0;

    // NEW MO2 INSTRUCTION TYPES
    // For READ/WRITE
    std::string memoryAddress;   // Used for READ and WRITE
};

