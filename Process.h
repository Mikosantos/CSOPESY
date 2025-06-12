#pragma once 
#include "Instruction.h"

#include <unordered_map>
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

        bool finished = false;

        static int NextProcessNum;

        // for instruction
        std::vector<Instruction> instructions;
        std::unordered_map<std::string, uint16_t> variables;

        int instructionPointer = 0;
        int sleepUntilTick = -1;

    public:
        Process(std::string& pName, int totalCom);

        //Getters
        std::string getTime();
        std::string getRawTime() const;
        std::string getProcessName();
        int getTotalNoOfCommands();
        int getCompletedCommands();
        int getCoreNo();
        int getProcessNo();
        int getNextProcessNum();

        bool isFinished();
        
        //Setters
        void setProcessName(const std::string& name);
        void setTotalNoOfCommands(int tCom);
        void setCompletedCommands(int cCom);
        void setCoreNum(int coreNum);
        void setProcessNum(int procNum);

        void setFinished(bool fin);

        //Auxilary 
        void displayScreen();

        // instruction
        void addInstruction(const Instruction& instr);
        bool executeInstruction(int coreId, int currentTick, std::ofstream& file);
        bool isSleeping(int currentTick) const;

        void declareVariable(const std::string& name, uint16_t value = 0);
        uint16_t getVariable(const std::string& name) const;
        void setVariable(const std::string& name, uint16_t value);

        // Optionally: Reset instruction pointer (for testing or reuse)
        // void resetInstructions();

        const Instruction& getCurrentInstruction() const;
        void advanceInstructionPointer();
        void setSleepUntil(int tick);

        int getInstructionPointer() const {
            return instructionPointer;
        }

        const std::vector<Instruction>& getInstructions() const {
            return instructions;
        }
};