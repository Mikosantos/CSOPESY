#pragma once 
#include "Instruction.h"
#include "InstructionUtils.h"

#include <unordered_map>
#include <string>
#include <fstream>
#include <chrono>
#include <mutex>

class Process {
    
    struct LoopContext {
        std::vector<Instruction> instructions;
        int repeatCount;
        unsigned long long currentRepeat;
        unsigned long long pointer;
    };
    std::vector<LoopContext> loopStack;
    
    private:
        std::string processName;
        unsigned long long totalNoOfCommands;
        unsigned long long completedCommands;
        int coreNum;
        int processNum;
        std::ofstream logFile;
        std::chrono::time_point<std::chrono::system_clock> time;

        bool finished = false;

        static int NextProcessNum;

        // for instruction
        std::vector<Instruction> instructions;
        std::unordered_map<std::string, uint16_t> variables;

        int instructionPointer = 0;
        int sleepUntilTick = -1;

        std::vector<std::string> logLines;

        mutable std::mutex coreNumMutex;

    public:
        Process(std::string& pName, int totalCom);

        //Getters
        std::string getTime();
        std::string getRawTime() const;
        std::string getProcessName();
        unsigned long long getTotalNoOfCommands();
        unsigned long long getCompletedCommands();
        int getCoreNo();
        int getProcessNo();
        int getNextProcessNum();
        bool isFinished();
        
        //Setters
        void setProcessName(const std::string& name);
        void setTotalNoOfCommands(unsigned long long tCom);
        void setCompletedCommands(unsigned long long cCom);
        void setCoreNum(int coreNum);
        void setProcessNum(int procNum);
        void setFinished(bool fin);

        // instruction
        void addInstruction(const Instruction& instr);
        bool executeInstruction(int coreId, int currentTick);
        bool isSleeping(int currentTick) const;

        void declareVariable(const std::string& name, uint16_t value = 0);
        uint16_t getVariable(const std::string& name) const;
        void setVariable(const std::string& name, uint16_t value);

        const Instruction& getCurrentInstruction() const;
        void advanceInstructionPointer();
        void setSleepUntil(int tick);
 
        unsigned long long getInstructionPointer() const;
        std::vector<Instruction> getInstructions() const;
        std::vector<std::string> getLogLines() const;
        void appendLogLine(const std::string& line);
        // ----------------------------------------------------------

        // to check if the process is still running
        bool isRunning() const;

        bool checkIfFinished();
};