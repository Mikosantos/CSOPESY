#include "Process.h"

#include <ctime>
#include <sstream>
#include <iostream>
#include <iomanip>

#define ORANGE "\033[38;5;208m"
#define BLUE   "\033[34m"
#define RESET  "\033[0m"

int Process::NextProcessNum = 1;

Process::Process(std::string& pName, int totalCom)
: processName(pName), totalNoOfCommands(totalCom) {
    time = std::chrono::system_clock::now();
    setCompletedCommands(0);
    setCoreNum(-1);
    setProcessNum(NextProcessNum++);
    setFinished(false);
};

// not sure but i think this is not needed
// consolePanel is the one that displays the screen
// void Process::displayScreen() {}

//getters ---------------------------------------------------
std::string Process::getTime() {
    auto now = std::chrono::system_clock::to_time_t(time);
    std::tm local_time;
    localtime_s(&local_time, &now);

    std::ostringstream oss;

    // Format: (MM/DD/YYYY HH:MM:SS PM)
    oss << BLUE << "(" << RESET;
    oss << ORANGE << std::setw(2) << std::setfill('0') << local_time.tm_mon + 1 << RESET;
    oss << BLUE << "/" << RESET;
    oss << ORANGE << std::setw(2) << std::setfill('0') << local_time.tm_mday << RESET;
    oss << BLUE << "/" << RESET;
    oss << ORANGE << (local_time.tm_year + 1900) << RESET;

    // Space
    oss << " ";

    // Hour
    int hour = local_time.tm_hour;
    std::string am_pm = "AM";
    if (hour >= 12) {
        am_pm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;
    oss << ORANGE << std::setw(2) << std::setfill('0') << hour << RESET;

    oss << BLUE << ":" << RESET;
    oss << ORANGE << std::setw(2) << std::setfill('0') << local_time.tm_min << RESET;
    oss << BLUE << ":" << RESET;
    oss << ORANGE << std::setw(2) << std::setfill('0') << local_time.tm_sec << RESET;

    // Space and AM/PM
    oss << " " << ORANGE << am_pm;

    oss << BLUE << ")" << RESET;

    return oss.str();
}

// Returns the raw time in the format (MM/DD/YYYY HH:MM:SS AM/PM) w/o colors
std::string Process::getRawTime() const {
    auto now = std::chrono::system_clock::to_time_t(time);
    std::tm local_time;
    localtime_s(&local_time, &now);

    std::ostringstream oss;

    // Format: (MM/DD/YYYY HH:MM:SS AM/PM)
    oss << "(";
    oss << std::setw(2) << std::setfill('0') << local_time.tm_mon + 1 << "/";
    oss << std::setw(2) << std::setfill('0') << local_time.tm_mday << "/";
    oss << (local_time.tm_year + 1900) << " ";

    int hour = local_time.tm_hour;
    std::string am_pm = "AM";
    if (hour >= 12) {
        am_pm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;

    oss << std::setw(2) << std::setfill('0') << hour << ":"
        << std::setw(2) << std::setfill('0') << local_time.tm_min << ":"
        << std::setw(2) << std::setfill('0') << local_time.tm_sec << " "
        << am_pm << ")";

    return oss.str();
}

std::string Process::getProcessName(){
    return this->processName;
}

int Process::getTotalNoOfCommands(){
    return this->totalNoOfCommands;
}
int Process::getCompletedCommands(){
    return this->completedCommands;
}
int Process::getCoreNo(){
    return this->coreNum;
}
int Process::getProcessNo(){
    return this->processNum;
}
int Process::getNextProcessNum(){
    return NextProcessNum;
}

bool Process::isFinished() {
    return finished;
}

//setters ----------------------------------------------------
void Process::setProcessName(const std::string& name){
    processName = name;
}
void Process::setTotalNoOfCommands(int tCom){
    totalNoOfCommands = tCom;
}
void Process::setCompletedCommands(int cCom){
    completedCommands = cCom;
}
void Process::setCoreNum(int cNum){
    coreNum = cNum;
}
void Process::setProcessNum(int procNum){
    processNum = procNum;
}

void Process::setFinished(bool fin) {
    finished = fin;
}

// INSTRUCTION RELATED FUNCTIONS ---------------------------------------------------
void Process::addInstruction(const Instruction& instr) {
    instructions.push_back(instr);
}

bool Process::isSleeping(int currentTick) const {
    return sleepUntilTick > currentTick;
}

bool Process::executeInstruction(int coreId, int currentTick) {
    Instruction instr;
    bool fromMainList = false;

    // Handle FOR loop stack
    if (!loopStack.empty()) {
        auto& loop = loopStack.back();

        // Finished current iteration?
        if (loop.pointer >= loop.instructions.size()) {
            loop.pointer = 0;
            loop.currentRepeat++;
        }

        // Exceeded loop repetition
        if (loop.currentRepeat >= loop.repeatCount) {
            loopStack.pop_back();
            return true;
        }

        instr = loop.instructions[loop.pointer++];
    } else {
        if (instructionPointer >= instructions.size()) return false;
        instr = instructions[instructionPointer++];
        fromMainList = true;
    }

    instr.executedTimestamp = generateCurrentTimestamp();
    instr.executedCore = coreId;

    // Write log
    std::ostringstream log;

    switch (instr.type) {
        case InstructionType::PRINT:            
            if (fromMainList) { 
                log << instr.executedTimestamp << "   Core: " << coreId << "   ";
                log << "PRINT \"Hello world from " << processName << "!\" \n";
                completedCommands++; 
            }

            break;

        case InstructionType::DECLARE:
            if (fromMainList) { 
                declareVariable(instr.var1, instr.value);
                log << instr.executedTimestamp << "   Core: " << coreId << "   ";
                log << "DECLARE " << instr.var1 << " = " << instr.value << "\n";
                completedCommands++;
            }

            break;

        case InstructionType::ADD: {   
            if (fromMainList) {
                uint16_t val2 = instr.var2IsImmediate ? instr.var2ImmediateValue : getVariable(instr.var2);
                uint16_t val3 = instr.var3IsImmediate ? instr.var3ImmediateValue : getVariable(instr.var3);

                setVariable(instr.var1, val2 + val3);

                log << instr.executedTimestamp << "   Core: " << coreId << "   ";
                log << "ADD " << instr.var1 << " = "
                    << (instr.var2IsImmediate ? std::to_string(val2) : instr.var2 + "/" + std::to_string(val2))
                    << " + "
                    << (instr.var3IsImmediate ? std::to_string(val3) : instr.var3 + "/" + std::to_string(val3))
                    << "\n";

                completedCommands++;
            }

            break;
        }

        case InstructionType::SUBTRACT: {
            if (fromMainList) {
                uint16_t val2 = instr.var2IsImmediate ? instr.var2ImmediateValue : getVariable(instr.var2);
                uint16_t val3 = instr.var3IsImmediate ? instr.var3ImmediateValue : getVariable(instr.var3);

                setVariable(instr.var1, val2 - val3);
                log << instr.executedTimestamp << "   Core: " << coreId << "   ";
                log << "SUBTRACT " << instr.var1 << " = "
                    << (instr.var2IsImmediate ? std::to_string(val2) : instr.var2 + "/" + std::to_string(val2))
                    << " - "
                    << (instr.var3IsImmediate ? std::to_string(val3) : instr.var3 + "/" + std::to_string(val3))
                    << "\n";
                
                completedCommands++;
            }

            break;
        }
        
        case InstructionType::SLEEP:
            if (fromMainList) {
                setSleepUntil(currentTick + instr.sleepTicks);
                log << instr.executedTimestamp << "   Core: " << coreId << "   ";
                log << "SLEEP for " << (int)instr.sleepTicks << " ticks \n";
                
                completedCommands++;
            }

            break;

        case InstructionType::FOR:
            if (fromMainList) {
                if (!instr.loopInstructions.empty() && instr.loopRepeat > 0) {
                    loopStack.push_back({instr.loopInstructions, instr.loopRepeat, 0, 0});
                    log << instr.executedTimestamp << "   Core: " << coreId << "   ";
                    log << "FOR loop start x" << instr.loopRepeat << "\n";

                    completedCommands++;
                } else {
                    log << "FOR loop invalid \n";
                }
            }

            break;
    }

    appendLogLine(log.str());
    instr.hasExecuted = true;
    return true;
}

void Process::declareVariable(const std::string& name, uint16_t value) {
    variables[name] = value;
}

uint16_t Process::getVariable(const std::string& name) const {
    auto it = variables.find(name);
    return (it != variables.end()) ? it->second : 0;
}

void Process::setVariable(const std::string& name, uint16_t value) {
    variables[name] = value;
}

const Instruction& Process::getCurrentInstruction() const {
    if (instructionPointer < instructions.size()) {
        return instructions[instructionPointer];
    } else {
        static Instruction dummy; // return default if out of bounds
        return dummy;
    }
}

void Process::advanceInstructionPointer() {
    if (instructionPointer < instructions.size())
        instructionPointer++;
}

void Process::setSleepUntil(int tick) {
    sleepUntilTick = tick;
}

int Process::getInstructionPointer() const {
    return instructionPointer;
}

std::vector<Instruction> Process::getInstructions() const {
    return instructions;
}

std::vector<std::string> Process::getLogLines() const {
    return logLines;
}

void Process::appendLogLine(const std::string& line) {
    logLines.push_back(line);
}

// TO DO: Implement resetInstructions to clear the instruction set and reset the pointer
// void Process::resetInstructions() {
//     instructionPointer = 0;
//     instructions.clear();
//     variables.clear();
// }

