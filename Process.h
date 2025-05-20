#pragma once 

#include <string>
#include <fstream>
#include <chrono>

class Process {
    private:
        std::string processName;
        int totalNoOfCommands;
        int completedCommands;
        int coreNum;
        int processNum;
        std::ofstream logFile;
        std::chrono::time_point<std::chrono::system_clock> time;

        static int NextProcessNum;

    public:
        Process(std::string& pName, int totalCom);

        //Getters
        std::string getTime();
        std::string getProcessName();
        int getTotalNoOfCommands();
        int getCompletedCommands();
        int getCoreNo();
        int getProcessNo();
        int getNextProcessNum();
        
        //Setters
        void setProcessName(const std::string& name);
        void setTotalNoOfCommands(int tCom);
        void setCompletedCommands(int cCom);
        void setCoreNum(int coreNum);
        void setProcessNum(int procNum);

        //Auxilary 
        void displayScreen();
};