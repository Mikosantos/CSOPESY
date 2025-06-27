#include "Console.h"
#include <iostream>
#include <ctime>
#include <string>

Console::Console(std:: string cName, unsigned long long cLine, unsigned long long tLines, int pId) :
    consoleName(cName), curLine(cLine), totalLines(tLines), processId(pId) {
}

std::string Console::getConsoleName() const { 
    return consoleName;
}

unsigned long long Console::getCurrentLine() const { 
    return curLine;
}

unsigned long long Console::getTotalLines() const { 
    return totalLines;
}

int Console::getProcessID() const { 
    return processId;
}

void Console::setConsoleName(const std::string& name) {
    consoleName = name; 
}

void Console::setCurrentLine(unsigned long long cLine) {
    curLine = cLine;
}

void Console::setTotalLines(unsigned long long tLines) {
    totalLines = tLines;
}

void Console::setProcessID(int pId) {
    processId = pId;
}