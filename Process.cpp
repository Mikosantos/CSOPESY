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
    setCoreNum(0);
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