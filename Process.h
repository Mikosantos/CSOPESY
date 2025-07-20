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

    int quantumUsed = 0;
    
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

        mutable std::mutex processMutex;

        int memSize = 0;  // in bytes
        int numPages;              // Number of virtual pages required

        std::vector<int> pageTable; // Maps virtual pages to frame numbers
                                    // -1 if page is not loaded (page fault will occur)

    public:
        Process(std::string& pName, int totalCom, int memSize);  // new signature

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

        // Atomic snapshot (to use for logging processList, ensures consistent reads
        // of multiple fields avoiding data races
        struct ProcessSnapshot {
        std::string processName;
        bool isRunning;
        int coreNo;
        unsigned long long completedCommands;
        unsigned long long totalNoCommands;
        std::string time;
        };

        ProcessSnapshot getAtomicSnapshot() const {
        std::lock_guard<std::mutex> lock(processMutex);
        return {
            processName,
            !finished && coreNum != -1,
            coreNum,
            completedCommands,
            totalNoOfCommands,
            getRawTime()
            };
        }
        // ----------------------------------------------------------

        // to check if the process is still running
        bool isRunning() const;

        bool checkIfFinished();

        int getQuantumUsed() const {
            return quantumUsed;
        }

        void resetQuantumUsed() {
            quantumUsed = 0;
        }

        void incrementQuantumUsed() {
            ++quantumUsed;
        }

        uint16_t simulateIORead(const std::string& varName);
        void simulateIOWrite(const std::string& varName, uint16_t value);

        // NEW MO2 FUNCTIONS ======================================
        int getMemSize() const { return memSize; }
        int getNumPages() const { return numPages; }
        const std::vector<int>& getPageTable() const { return pageTable; }

        void initializePages(int memPerFrame); // New function to compute pages & initialize pageTable
        void setPageFrame(int pageIndex, int frameNo); // Update page table mapping

        bool isPageLoaded(int pageIndex) const; // Check if the page is in memory
};