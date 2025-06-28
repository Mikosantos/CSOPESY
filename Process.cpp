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

// getters ---------------------------------------------------
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

// Returns the raw time in the format (MM/DD/YYYY HH:MM:SS AM/PM) w/o colors for logs
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

unsigned long long Process::getTotalNoOfCommands(){
    return this->totalNoOfCommands;
    // return countExpandedInstructions(instructions);
}

unsigned long long Process::getCompletedCommands(){
    return this->completedCommands;
}

int Process::getCoreNo(){
    std::lock_guard<std::mutex> lock(processMutex);
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

// setters ----------------------------------------------------
void Process::setProcessName(const std::string& name){
    std::lock_guard<std::mutex> lock(processMutex);
    processName = name;
}

void Process::setTotalNoOfCommands(unsigned long long tCom){
    std::lock_guard<std::mutex> lock(processMutex);
    totalNoOfCommands = tCom;
}

void Process::setCompletedCommands(unsigned long long cCom){
    std::lock_guard<std::mutex> lock(processMutex);
    completedCommands = cCom;
}

void Process::setCoreNum(int cNum){
    std::lock_guard<std::mutex> lock(processMutex);
    coreNum = cNum;
}

void Process::setProcessNum(int procNum){
    std::lock_guard<std::mutex> lock(processMutex);
    processNum = procNum;
}

void Process::setFinished(bool fin) {
    std::lock_guard<std::mutex> lock(processMutex);
    finished = fin;
}

// INSTRUCTION RELATED FUNCTIONS -------------------------------

// New method to check if process is finished based on instructions and loop stack
bool Process::checkIfFinished() {
    // We lock processMutex to synchronize access to finished and coreNum
    std::lock_guard<std::mutex> lock(processMutex);

    // Finish if completed commands reached or
    // instruction pointer is at end and no loops remain
    if (completedCommands >= totalNoOfCommands) {
        finished = true;
        return true;
    }
    if (instructionPointer >= instructions.size() && loopStack.empty()) {
        finished = true;
        return true;
    }
    return false;
}

/*
    Adds an instruction to the process.
    This function appends the given instruction to the process's instruction list.
*/
void Process::addInstruction(const Instruction& instr) {
    instructions.push_back(instr);
}

// Check if the process is currently sleeping (due to a SLEEP instruction).
bool Process::isSleeping(int currentTick) const {
    return sleepUntilTick > currentTick;
}

/*
    Executes instruction in the process.
    This function retrieves the next instruction from the process's instruction list
    and executes it based on its type. It also handles different instructions, including nested for loops and updates the
    process's state accordingly.
*/
bool Process::executeInstruction(int coreId, int currentTick) {
    Instruction instr;

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
            // After popping, call checkIfFinished in case process done now
            checkIfFinished();
            return true;
        }

        instr = loop.instructions[loop.pointer++];
    } else {
        if (instructionPointer >= instructions.size()) {
            // No instructions left
            checkIfFinished();
            return false;
        }
        instr = instructions[instructionPointer++];
    }

    instr.executedTimestamp = generateCurrentTimestamp();
    instr.executedCore = coreId;

    std::ostringstream log;
    // Execute instruction and increment completedCommands for every executed instruction
    switch (instr.type) {
        case InstructionType::PRINT:            
            log << instr.executedTimestamp << "   Core: " << coreId << "   ";
            // log << "\"Hello world from " << processName << "!\" \n";
            log << "Value from " << instr.var1 << ": " << getVariable(instr.var1) << "\n";
            completedCommands++; 
            break;

        case InstructionType::DECLARE:
            declareVariable(instr.var1, instr.value);
            completedCommands++;
            break;

        case InstructionType::ADD: {   
            uint16_t val2 = instr.var2IsImmediate ? instr.var2ImmediateValue : getVariable(instr.var2);
            uint16_t val3 = instr.var3IsImmediate ? instr.var3ImmediateValue : getVariable(instr.var3);
            setVariable(instr.var1, val2 + val3);
            completedCommands++;
            break;
        }

        case InstructionType::SUBTRACT: {
            uint16_t val2 = instr.var2IsImmediate ? instr.var2ImmediateValue : getVariable(instr.var2);
            uint16_t val3 = instr.var3IsImmediate ? instr.var3ImmediateValue : getVariable(instr.var3);
            setVariable(instr.var1, val2 - val3);
            completedCommands++;
            break;
        }
        
        case InstructionType::SLEEP:
            setSleepUntil(currentTick + instr.sleepTicks);
            completedCommands++;
            break;

        case InstructionType::FOR:
            // Push loop instructions and repetitions onto stack if valid
            if (!instr.loopInstructions.empty() && instr.loopRepeat > 0) {
                loopStack.push_back({instr.loopInstructions, instr.loopRepeat, 0, 0});
                // DO NOT increment completedCommands here because the loop body will be counted
            }
            break;
    }

    appendLogLine(log.str());
    instr.hasExecuted = true;

    // After executing an instruction, check if process is finished
    checkIfFinished();

    return true;
}

// Declare a variable with an optional initial value
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

// Get the current instruction based on the instruction pointer.
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

unsigned long long Process::getInstructionPointer() const {
    return instructionPointer;
}

std::vector<Instruction> Process::getInstructions() const {
    return instructions;
}

// Get the log lines generated by the process
// This function returns a vector of strings containing the log lines.
// Use for displaying the process's execution history.
std::vector<std::string> Process::getLogLines() const {
    return logLines;
}

// Append a log line to the process's log
void Process::appendLogLine(const std::string& line) {
    logLines.push_back(line);
}

// Check if the process is currently running
// A process is considered running if it is not finished and has a valid core number assigned.
bool Process::isRunning() const {
    return !finished && coreNum != -1;
}