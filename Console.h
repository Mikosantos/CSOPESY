#pragma once
#include <string>


class Console {
    private:
        std::string consoleName;
        std::string curTime;
        unsigned long long curLine;
        unsigned long long totalLines;
        int processId;
        
    public: 
        Console(std::string consoleName, unsigned long long curLine, unsigned long long totalLines, int processId);
        std::string getConsoleName() const;
        unsigned long long getCurrentLine() const;
        unsigned long long getTotalLines() const;
        int getProcessID() const;

        void setConsoleName(const std::string& name);
        void setCurrentLine(unsigned long long cLine);
        void setTotalLines(unsigned long long tLines);
        void setProcessID(int pId);
};