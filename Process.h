#pragma once 
#include "Instruction.h"
#include "InstructionUtils.h"
#include "MemoryManager.h"

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

        // NEW MO2 FIELDS =================================================================================
        size_t  memSize = 0;           // in bytes (this is memory required)
        size_t  numPages;              // Number of virtual pages required

        std::vector<int> pageTable; // Maps virtual pages to frame numbers
                                    // -1 if page is not loaded (page fault will occur)

        bool memoryViolation = false;
        std::string violationTime;
        size_t violationAddress; // The invalid memory address accessed

        std::shared_ptr<MemoryManager> memManager;

        bool memoryInitialized = false;

        std::unordered_map<std::string, int> symbolTableOffsets;
        const int SYMBOL_TABLE_START = 0x0000;
        const int SYMBOL_TABLE_SIZE = 64; // bytes

    public:
        Process(std::string& pName, int totalCom, size_t memSize, std::shared_ptr<MemoryManager> memManager);  // new signature

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

        void writeToMemory(int virtualAddress, uint16_t value);
        uint16_t readFromMemory(int virtualAddress);

        // NEW MO2 FUNCTIONS ======================================
        size_t  getMemSize() const { return memSize; }
        size_t  getNumPages() const { return numPages; }
        const std::vector<int>& getPageTable() const { return pageTable; }

        void initializePages(size_t  memPerFrame); // New function to compute pages & initialize pageTable
        void setPageFrame(size_t  pageIndex, int frameNo); // Update page table mapping

        bool isPageLoaded(size_t  pageIndex) const; // Check if the page is in memory

        void setMemoryViolation(size_t address) {
            memoryViolation = true;
            violationTime = getRawTime();
            violationAddress = address;
            setFinished(true);
        }

        bool hasMemoryViolation() const {
            return memoryViolation;
        }

        std::string getViolationTime() const {
            return violationTime;
        }

        size_t getViolationAddress() const {
            return violationAddress;
        }

        size_t getMemorySize() const { return memSize; }

        bool isMemoryInitialized() const { return memoryInitialized; }
        void markMemoryInitialized() { memoryInitialized = true; }

        void setMemoryInitialized(bool initialized) {
            memoryInitialized = initialized;
        }

        bool isDeclared(const std::string& name) const {
            return symbolTableOffsets.find(name) != symbolTableOffsets.end();
        }

};