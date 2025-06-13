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

//TODO
void Process::displayScreen(){

}

//getters
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

//setters
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

// instruction
std::string processTimeStamp() {
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

void Process::addInstruction(const Instruction& instr) {
    instructions.push_back(instr);
}

bool Process::isSleeping(int currentTick) const {
    return sleepUntilTick > currentTick;
}

bool Process::executeInstruction(int coreId, int currentTick) {
    if (instructionPointer >= instructions.size()) return false;

    const Instruction& instr = instructions[instructionPointer];

    // Write log header
    std::ostringstream log;
    log << processTimeStamp() << "   Core: " << coreId << "   ";

    switch (instr.type) {
        case InstructionType::PRINT:
            log << "PRINT \"Hello world from " << processName << "!\"\n";
            break;
        case InstructionType::DECLARE:
            log << "DECLARE " << instr.var1 << " = " << instr.value << "\n";
            declareVariable(instr.var1, instr.value);
            break;
        case InstructionType::ADD:
            log << "ADD " << instr.var1 << " = "
                << instr.var2 << "/" << getVariable(instr.var2) << " + "
                << instr.var3 << "/" << getVariable(instr.var3) << "\n";
            setVariable(instr.var1, getVariable(instr.var2) + getVariable(instr.var3));
            break;
        case InstructionType::SUBTRACT:
            log << "SUBTRACT " << instr.var1 << " = "
                << instr.var2 << "/" << getVariable(instr.var2) << " - "
                << instr.var3 << "/" << getVariable(instr.var3) << "\n";
            setVariable(instr.var1, getVariable(instr.var2) - getVariable(instr.var3));
            break;
        case InstructionType::SLEEP:
            log << "SLEEP for " << static_cast<int>(instr.sleepTicks) << " ticks" << "\n";
            setSleepUntil(currentTick + instr.sleepTicks);
            break;
    }

    appendLogLine(log.str());


    // Update internal state
    instructionPointer++;
    completedCommands++;

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

// TO DO: Implement resetInstructions to clear the instruction set and reset the pointer

// void Process::resetInstructions() {
//     instructionPointer = 0;
//     instructions.clear();
//     variables.clear();
// }

