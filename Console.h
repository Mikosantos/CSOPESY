#pragma once
#include <string>


class Console {
    private:
        std::string consoleName;
        std::string curTime;
        int curLine;
        int totalLines;
        int processId;
        
    public: 
        Console(std::string consoleName, int curLine, int totalLines, int processId);
        std::string getTimestamp();
        std::string getConsoleName() const;
        int getCurrentLine() const;
        int getTotalLines() const;
        int getProcessID() const;

        void setConsoleName(const std::string& name);
        void setCurrentLine(int cLine);
        void setTotalLines(int tLines);
        void setProcessID(int pId);
};