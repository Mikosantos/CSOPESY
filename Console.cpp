#include "Console.h"
#include <iostream>
#include <ctime>
#include <string>

Console::Console(std:: string cName, int cLine, int tLines, int pId) :
    consoleName(cName), curLine(cLine), totalLines(tLines), processId(pId){
        curTime = getTimestamp();
}
std::string Console::getTimestamp() {
	time_t timestamp;
	tm * tm_stamp;
	char ts_string[100];
	std::string strTimeStamp;	
	time(&timestamp);
	tm_stamp = localtime(&timestamp);
			
	std::strftime(ts_string, 100, "%m/%d/%Y, %I:%M:%S %p", tm_stamp);
	strTimeStamp = ts_string;
    return strTimeStamp;
}
std::string Console::getConsoleName() const { 
    return consoleName;
}
int Console::getCurrentLine() const { 
    return curLine;
}
int Console::getTotalLines() const { 
    return totalLines;
}
int Console::getProcessID() const { 
    return processId;
}

void Console::setConsoleName(const std::string& name) {
    consoleName = name; 
}
void Console::setCurrentLine(int cLine) {
    curLine = cLine;
}
void Console::setTotalLines(int tLines) {
    totalLines = tLines;
}
void Console::setProcessID(int pId) {
    processId = pId;
}