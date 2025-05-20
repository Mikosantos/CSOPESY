#include "Process.h"

#include <ctime>
#include <sstream>
#include <iostream>
#include <iomanip>

int Process::NextProcessNum = 1;

Process::Process(std::string& pName, int totalCom)
: processName(pName), totalNoOfCommands(totalCom) {
    time = std::chrono::system_clock::now();
    setCompletedCommands(0);
    setCoreNum(-1);
    setProcessNum(NextProcessNum++);
};

//TODO
void Process::displayScreen(){

}

//getters
std::string Process::getTime(){
    auto now = std::chrono::system_clock::to_time_t(time);
    std::tm local_time;
    localtime_s(&local_time, &now);
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%m/%d/%Y %I:%M:%S%p");
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