/* Header Files */
#include "Console.h"
#include "ConsolePanel.h"
#include "Process.h"
#include "Config.h"
#include "Scheduler.h"
#include "InstructionUtils.h"
#include "Scheduler.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"
#include "utils.h"
#include "MemoryManager.h"

/* Libraries */
#include <string>
#include <iostream>
#include <random>
#include <windows.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <chrono>

#define ORANGE "\033[38;5;208m"
#define RESET  "\033[0m"
#define CYAN   "\033[38;5;51m"
#define BLUE   "\033[34m"
#define LIGHT_RED "\033[91m"

using namespace std;

// function declarations
void handleMainScreenCommands(const string& cmd, const vector<string>& args, ConsolePanel& consolePanel, vector<shared_ptr<Process>>& processList, 
                              bool& hasInitialized, bool& notShuttingDown);
void handleProcessScreenCommands(const string& cmd, const string& currentScreenName, const vector<shared_ptr<Process>>& processList, ConsolePanel& consolePanel);
void setColor(unsigned char color);
void header();
pair<string, vector<string>> parseCommand(const string& input);
void initialize();
void scheduler_start(std::vector<std::shared_ptr<Process>>& processList, ConsolePanel& consolePanel);
void scheduler_stop();
void report_util(const std::vector<std::shared_ptr<Process>>& allProcesses, const std::vector<std::shared_ptr<Process>>& runningProcesses);
void printSystemSummary(Scheduler* scheduler, std::shared_ptr<MemoryManager> memManager);
void printHelpMenu();
void handleExit();
void clear();
void clearToProcessScreen();
void displayProcessScreen(const std::shared_ptr<Process>& proc);
void printLastUpdated();
void startBatchGeneration(std::vector<std::shared_ptr<Process>>&, ConsolePanel&);
void stopBatchGeneration();
string trim(const string& str);

std::unique_ptr<Scheduler> scheduler;
Config config;

std::atomic<bool> isBatchGenerating = false;
std::thread batchGeneratorThread;
std::atomic<int> batchProcessCount = 0;
int processCounter = 1;

std::shared_ptr<MemoryManager> memoryManager;

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    string input;
    ConsolePanel consolePanel;
    bool notShuttingDown = true;
    bool hasInitialized = false;
    vector<shared_ptr<Process>> processList;

    header();

    while (notShuttingDown) {
        cout << "root:\\> ";
        getline(cin, input);

        auto [cmd, args] = parseCommand(input);

        string currentScreen = consolePanel.getCurrentScreenName();

        if (cmd != "initialize" && cmd != "exit" && !hasInitialized) {
            cout << "Initialize the program with command \"initialize\" first!\n\n";
            continue;
        }

        if (currentScreen == "MAIN_SCREEN") {
            handleMainScreenCommands(cmd, args, consolePanel, processList, hasInitialized, notShuttingDown);
        } else {
            handleProcessScreenCommands(cmd, currentScreen, processList, consolePanel);
        }
    }
    return 0;
}

void handleMainScreenCommands(const string& cmd, const vector<string>& args, ConsolePanel& consolePanel,
                              vector<shared_ptr<Process>>& processList, bool& hasInitialized, bool& notShuttingDown) {
    auto screens = consolePanel.getConsolePanels();

    if (cmd == "exit") {
        notShuttingDown = false;

        if(scheduler != nullptr)
            scheduler->stop();
        
        handleExit();
    } 
    
    else if (cmd == "initialize") {
        if (hasInitialized) {
            cout << "System has already been initialized.\n\n";
        } else {
            hasInitialized = true;
            initialize();

        }
    } 
    
    else if (cmd == "clear") {
        clear();
    } 
    
    else if (cmd == "help") {
        printHelpMenu();
    } 
    
    else if (cmd == "scheduler-start") {
        scheduler_start(processList, consolePanel);
    } 
    
    else if (cmd == "scheduler-stop") {
        scheduler_stop();
    } 
    
    else if (cmd == "report-util") {
        report_util(processList, scheduler->getRunningProcesses());
    } 

    // MO2 NEW COMMANDS ===========
    else if (cmd == "process-smi") {
        constexpr size_t KIB = 1024;
        constexpr size_t MIB = KIB * 1024;
        
        // Byte
        size_t usedMem = memoryManager->getUsedMemoryBytes();
        size_t totalMem = config.maxOverallMemory;

        // MiB
        // double usedMiB = usedMem / (1024.0 * 1024.0);
        // double totalMiB = config.maxOverallMemory / (1024.0 * 1024.0);

        // KiB
        // double usedKiB = usedMem / static_cast<double>(KIB);
        // double totalKiB = config.maxOverallMemory / static_cast<double>(KIB);

        int totalCores = scheduler->getTotalCoreCount();
        int busy = 0;

        for (int core = 0; core < totalCores; ++core) {
            auto process = scheduler->getProcessOnCore(core);
            if (process && memoryManager->getProcessUsedMemory(process->getProcessNo()) > 0) {
                busy++;
            }
        }
        auto runningProcesses = scheduler->getRunningProcesses();

        int cpuUtil = (static_cast<double>(busy) / totalCores) * 100;
        int memUtil = static_cast<int>((static_cast<double>(usedMem) / config.maxOverallMemory) * 100);

        std::cout << "\n";
        std::cout << "+----------------------------------------------------+\n";
        std::cout << "|      PROCESS-SMI V01." << ORANGE "00   " << RESET << "DRIVER VERSION: 01." << ORANGE << "00" << RESET << "    |\n";
        std::cout << "+----------------------------------------------------+\n";
        std::cout << "CPU-Util      : " << cpuUtil  << BLUE   << "%\n"  << RESET;
        std::cout << "Memory Usage  : " << usedMem  << ORANGE <<" Byte" << BLUE << " / " << RESET << totalMem << ORANGE << " Byte\n" << RESET;
        std::cout << "Memory Util   : " << memUtil  << BLUE   << "%\n\n"<< RESET;

        std::cout << "======================================================\n";
        std::cout << "Running processes " << BLUE "and"  << RESET << " memory usage:\n";
        std::cout << "+----------------------------------------------------+\n";
            consolePanel.listMemoryUsageOfRunningProcesses(runningProcesses, memoryManager);
        std::cout << "+----------------------------------------------------+\n\n";
    }

    else if (cmd == "vmstat") {
        std::cout << "\n";
        std::cout << "+----------------------------------------------------+\n";
        std::cout << "|                       VMSTAT                       |\n";
        std::cout << "+----------------------------------------------------+\n";

        size_t totalMemory = memoryManager->getTotalMemory();          // in bytes
        size_t usedMemory  = memoryManager->getUsedMemoryBytes();      // in bytes
        size_t freeMemory  = totalMemory - usedMemory;

        int pagedIn  = memoryManager->getPagesPagedIn();
        int pagedOut = memoryManager->getPagesPagedOut();              // pages

        uint64_t totalCpuTicks = scheduler->getTotalCpuTicks();        // total across all cores
        uint64_t idleCpuTicks  = scheduler->getIdleCpuTicks();
        uint64_t activeCpuTicks = totalCpuTicks - idleCpuTicks;

        std::cout << ORANGE << totalMemory << RESET << " total Memory\n";
        std::cout << ORANGE << usedMemory  << RESET << " used Memory\n";
        std::cout << ORANGE << freeMemory  << RESET << " free Memory\n\n";

        std::cout << ORANGE << idleCpuTicks   << RESET << " Idle CPU Ticks\n";
        std::cout << ORANGE << activeCpuTicks << RESET << " Active CPU Ticks\n";
        std::cout << ORANGE << totalCpuTicks  << RESET << " Total CPU Ticks\n\n";

        std::cout << ORANGE << pagedIn  << RESET << " Num Paged In\n";
        std::cout << ORANGE << pagedOut << RESET << " Num Paged Out\n";
        std::cout << "+----------------------------------------------------+\n\n";

    }
    // 
    
    else if (cmd == "screen" && args.size() == 1 && args[0] == "-ls") {
        printSystemSummary(scheduler.get(), memoryManager);
        std::vector<std::shared_ptr<Process>> trulyRunning;
        for (const auto& proc : scheduler->getRunningProcesses()) {
            if (memoryManager->getProcessUsedMemory(proc->getProcessNo()) > 0) {
                trulyRunning.push_back(proc);
            }
        }
        consolePanel.listProcesses(processList, trulyRunning);

    } 
    
    else if (cmd == "screen" && args.size() >= 3 && args[0] == "-s") {
        string procName = args[1];
        string memSizeStr = args[2];

        for (const auto& c : screens) {
            if (c->getConsoleName() == procName) {
                cout << "Process '" << procName << "' already exists. Use -r to resume.\n\n";
                return;
            }
        }

        unsigned int memSize;
        try {
            memSize = std::stoi(memSizeStr);
        } catch (const std::exception&) {
            cout << "Invalid memory size format.\n\n";
            return;
        }

        if (memSize < 64 || memSize > 65536 || (memSize & (memSize - 1)) != 0) {
            cout << "Invalid memory allocation! Must be a power of 2 between 64 and 65536.\n\n";
            return;
        }

        unsigned long long total = config.minInstructions + rand() % (config.maxInstructions - config.minInstructions + 1);

        clearToProcessScreen();
        auto newProc = make_shared<Process>(procName, total, memSize, memoryManager); // new

        auto instructions = generateRandomInstructions(total, procName, memSize, config);
        for (const auto& instr : instructions) {
            newProc->addInstruction(instr);
        }

        processList.push_back(newProc);

        auto procConsole = make_shared<Console>(procName, 0, total, newProc->getProcessNo());
        consolePanel.addConsolePanel(procConsole);
        consolePanel.setCurrentScreen(procConsole);

        displayProcessScreen(newProc);

        scheduler->addProcess(newProc);
    } 
    
    else if (cmd == "screen" && args.size() >= 2 && args[0] == "-r") {
        string procName = args[1];
        bool foundScreen = false, foundProcess = false;
        std::shared_ptr<Process> targetProcess = nullptr;
        std::shared_ptr<Console> currentPanel = nullptr;

        for (auto& s : screens) {
            if (s->getConsoleName() == procName) {
                foundScreen = true;
                currentPanel = s;
                break;
            }
        }

        for (auto& p : processList) {
            if (p->getProcessName() == procName) {
                foundProcess = true;
                targetProcess = p;
                break;
            }
        }

        if (!foundProcess) {
            cout << "Process '" << procName << "' not found.\n\n";
            return;
        }

        if (targetProcess->hasMemoryViolation()) {
            std::ostringstream oss;
            oss << "Process '" << procName << "' shut down due to memory access violation error that occurred at ";
            oss << targetProcess->getViolationTime() << ". ";
            oss << "0x" << std::hex << targetProcess->getViolationAddress() << " invalid.\n\n";
            cout << oss.str();
            return;
        }

        if (!foundScreen || targetProcess->isFinished()) {
            cout << "Process '" << procName << "' not found.\n\n";
            return;
        }
        
        clearToProcessScreen();
        consolePanel.setCurrentScreen(currentPanel);
        displayProcessScreen(targetProcess);

    } 

    // NEW MO2 COMMAND
    else if (cmd == "screen" && args.size() >= 3 && args[0] == "-c") {
        string procName = args[1];
        string memSizeStr = args[2];

        unsigned int memSize;
        try {
            memSize = stoi(memSizeStr);
        } catch (const exception&) {
            cout << "Invalid memory size format.\n\n";
            return;
        }

        if (memSize < 64 || memSize > 65536 || (memSize & (memSize - 1)) != 0) {
            cout << "Invalid memory allocation! Must be a power of 2 between 64 and 65536.\n\n";
            return;
        }

        // Combine remaining arguments into a single instruction string
        string instructionString;
        for (size_t i = 3; i < args.size(); ++i) {
            instructionString += args[i] + " ";
        }

        // Trim and remove surrounding quotes
        instructionString = trim(instructionString);
        if (!instructionString.empty() && instructionString.front() == '"' && instructionString.back() == '"') {
            instructionString = instructionString.substr(1, instructionString.size() - 2);
        }

        // Smarter semicolon split: only split outside quotes
        std::vector<std::string> rawInstructions;
        std::string currentInstr;
        bool insideQuotes = false;

        for (char c : instructionString) {
            if (c == '"') {
                insideQuotes = !insideQuotes;
            }

            if (c == ';' && !insideQuotes) {
                std::string trimmed = trim(currentInstr);
                if (!trimmed.empty()) rawInstructions.push_back(trimmed);
                currentInstr.clear();
            } else {
                currentInstr += c;
            }
        }

        if (!currentInstr.empty()) {
            std::string trimmed = trim(currentInstr);
            if (!trimmed.empty()) rawInstructions.push_back(trimmed);
        }

        // Check instruction count
        if (rawInstructions.size() < 1 || rawInstructions.size() > 50) {
            cout << "Invalid command. Instruction count must be between 1 and 50.\n\n";
            return;
        }

        // Create process
        clearToProcessScreen();
        auto newProc = make_shared<Process>(procName, rawInstructions.size(), memSize, memoryManager);
        
        // Parse and add fixed instructions
        auto fixedInstructions = generateFixedInstructions(rawInstructions);
        for (const auto& instr : fixedInstructions) {
            newProc->addInstruction(instr);
        }

        processList.push_back(newProc);

        auto procConsole = make_shared<Console>(procName, 0, rawInstructions.size(), newProc->getProcessNo());
        consolePanel.addConsolePanel(procConsole);
        consolePanel.setCurrentScreen(procConsole);

        displayProcessScreen(newProc);

        scheduler->addProcess(newProc);
    }
    // 

    // ADDITIONAL FEATURE =======
    else if (cmd == "print-ready") {
        auto readyList = scheduler->getReadyQueueSnapshot();

        std::cout << "\n+----------------------------------------------------+\n";
        std::cout << "|  Ready queue count: " << ORANGE << readyList.size() << RESET << "\n";

        if (readyList.empty()) {
            std::cout << "+----------------------------------------------------+\n";
            std::cout << LIGHT_RED << "|  Ready queue is empty.                             |\n" << RESET;
            std::cout << "+----------------------------------------------------+\n\n";
        } else {
            std::cout << "+----------------------------------------------------+\n";
            std::cout << BLUE << "| Current Ready Queue:                               |\n" << RESET;
            std::cout << "+----------------------------------------------------+\n";
            for (const auto& proc : readyList) {
                std::cout << "Process: " << ORANGE << proc->getProcessName() << RESET
                        << " (PID " << proc->getProcessNo() << "), "
                        << "Completed: " << proc->getCompletedCommands()
                        << " / " << proc->getTotalNoOfCommands()
                        << "\n";
            }
            std::cout << "+----------------------------------------------------+\n\n";
        }
    }
    // =======

    else {
        cout << "Unknown command! Type \"help\" for commandlist.\n\n";
    }
}

void handleProcessScreenCommands(const string& cmd, const string& currentScreenName, const vector<shared_ptr<Process>>& processList, ConsolePanel& consolePanel) {
    auto screens = consolePanel.getConsolePanels();
    
    if (cmd == "exit") {
        cout << "\033c" << flush;
        for (auto& screenPtr : screens) {
                if (screenPtr->getConsoleName() == "MAIN_SCREEN") {
                    consolePanel.setCurrentScreen(screenPtr);
                    break;
                }
            }
            if (consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
                clear();
            }
    } 
    
    else if (cmd == "process-smi") {
        for (auto& p : processList) {
            if (p->getProcessName() == currentScreenName) {
                displayProcessScreen(p);
                break;
            }
        }
    } 
    
    else {
        cout << "Only 'exit' and 'process-smi' commands are allowed inside a process screen.\n\n";
    }
}

void displayProcessScreen(const std::shared_ptr<Process>& proc) {
    std::string logsDir = "processLogs";
    std::string logFilePath = logsDir + "/" + proc->getProcessName() + ".txt";

    cout << "\n=====================================================\n";
    setColor(0x02); //color green
    cout << "                  PROCESS CONSOLE SCREEN             \n";
    setColor(0x07); // default
    cout << "=====================================================\n";
    cout << "Process name: " << proc->getProcessName() << "\n";
    cout << "ID: " << ORANGE << proc->getProcessNo() << RESET << "\n";
    cout << "Logs:\n\n";

    // print each instruction logs
    const auto& logs = proc->getLogLines();

    for (const auto& line : logs) {
        std::cout << line;
    }

    std::cout << "\n";

    // Progress / Completion message
    if (proc->isFinished()) {
        std::cout << ORANGE << "Finished!" << RESET << "\n";
    } else {
        std::cout << "Current instruction line: " << ORANGE << proc->getCompletedCommands() << RESET << "\n";
        std::cout << "Lines of instruction: " << ORANGE << proc->getTotalNoOfCommands() << RESET << "\n";
    }

    std::cout << "=====================================================\n";
}

void setColor( unsigned char color ){
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), color );
}

void printLastUpdated() {
    namespace fs = std::filesystem;

    std::string path = (fs::current_path() / "main.cpp").string();

    //std::cout << "Current path: " << path << "\n";

    try
    {
        auto ftime = fs::last_write_time(path);

        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );

        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

        std::cout << "Last updated: " 
                  << std::put_time(std::localtime(&cftime), "%m/%d/%Y %I:%M:%S %p") 
                  << std::endl;
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void header() {
    setColor(0x07);
    cout << "  ____ ____  ____  _____ _____ ____ __   __     " << endl;
    cout << " / __/  ___|/ __ `|  _  ` ____/ ___`  ` / /     " << endl;
    cout << "| |   `___ ` |  | | |_| |  __|`___ ` `   /      " << endl;
    cout << "| |__ ___) | |__| | ___/| |___ ___) | | |       " << endl;
    cout << " `___` ____/`____/|_|   |_____|___ /  |_|       " << endl;
    cout << "--------------------------------------------------\n";
    setColor(0x02);
    cout << "Hello, Welcome to CSOPESY commandline!\n\n";

    setColor(0x07);
    cout << "Developers:\n";
    cout << "Albarracin, Clarissa\n";
    cout << "Garcia, Reina Althea\n";
    cout << "Santos, Miko\n\n";

    printLastUpdated();
    cout << "\n\n";

    setColor(0x0E);
    cout << "Type 'exit' to quit, 'clear' to clear the screen\n"; 
    cout << "--------------------------------------------------\n";
    setColor(0x07);
}

pair<string, vector<string>> parseCommand(const string& input) {
	istringstream stream(input);
	string cmd;
	stream >> cmd;

	vector<string> args;
	string arg;
	
	while (stream >> arg) {
		args.push_back(arg);
	}

	return {cmd, args};
}

void initialize() {
    // delete any existing previous logs
    std::string consoleLogFile = "csopesy-log.txt";

    try {
        bool isDeleted = false;
        
        // Delete console-log.txt if it exists
        if (std::filesystem::exists(consoleLogFile)) {
            std::filesystem::remove(consoleLogFile);
            isDeleted = true;
        }
        
        if (isDeleted) {
            std::cout << "Deleted previous log files.\n\n";
        }
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error deleting files: " << e.what() << std::endl;
    }


    config = loadConfig("config.txt");

    std::cout << ORANGE << "[Initializing System...]\n" << RESET;

    std::cout << "Loaded configuration:\n";
    std::cout << "  Scheduler Type     : " << ORANGE << config.schedulerType    << RESET << "\n";
    std::cout << "  Number of CPUs     : " << ORANGE << config.numCPUs          << RESET << "\n";
    std::cout << "  Quantum Cycles     : " << ORANGE << config.quantumCycles    << RESET << "\n";
    std::cout << "  Batch Process Freq : " << ORANGE << config.batchProcessFreq << RESET << "\n";
    std::cout << "  Min Instructions   : " << ORANGE << config.minInstructions  << RESET << "\n";
    std::cout << "  Max Instructions   : " << ORANGE << config.maxInstructions  << RESET << "\n";
    std::cout << "  Delay Per Exec     : " << ORANGE << config.delaysPerExec    << RESET << "\n\n";

    std::cout << "  Max Overall Mem    : " << ORANGE << config.maxOverallMemory << RESET << "\n";
    std::cout << "  Mem Per Frame      : " << ORANGE << config.memPerFrame      << RESET << "\n";
    std::cout << "  Min Mem Per Proc   : " << ORANGE << config.minMemPerProcess << RESET << "\n";
    std::cout << "  Max Mem Per Proc   : " << ORANGE << config.maxMemPerProcess << RESET << "\n";

    std::cout << "\nStarting scheduler...\n";

    if (config.schedulerType == "fcfs") {
        memoryManager = std::make_shared<MemoryManager>(config.maxOverallMemory, config.memPerFrame);
        scheduler = std::make_unique<FCFSScheduler>(config.numCPUs, config.delaysPerExec, config, memoryManager);
        scheduler->start();
        // memoryManager = std::make_shared<MemoryManager>(config.maxOverallMemory, config.memPerFrame);
        std::cout << ORANGE << "[FCFS Scheduler started with "
                  << config.numCPUs << " cores]" << RESET << "\n\n";
    } 
    
    else if (config.schedulerType == "rr") {
        memoryManager = std::make_shared<MemoryManager>(config.maxOverallMemory, config.memPerFrame);
        scheduler = std::make_unique<RRScheduler>(config.numCPUs, config.delaysPerExec, config.quantumCycles, config, memoryManager);
        scheduler->start();
        // memoryManager = std::make_shared<MemoryManager>(config.maxOverallMemory, config.memPerFrame);
        std::cout << ORANGE << "[RR Scheduler started with "
                  << config.numCPUs << " cores]" << RESET << "\n\n";
    } 
    
    else {
        std::cout << "Invalid scheduler type in config file.\n\n";
        return;
    }
}

void scheduler_start(std::vector<std::shared_ptr<Process>>& processList, ConsolePanel& consolePanel) {
	startBatchGeneration(processList, consolePanel);
}

void scheduler_stop() {
	stopBatchGeneration();
}

void report_util(const std::vector<std::shared_ptr<Process>>& allProcesses,
                const std::vector<std::shared_ptr<Process>>& runningProcesses) {

    std::filesystem::path logPath = std::filesystem::current_path() / "csopesy-log.txt";
    std::ofstream log("csopesy-log.txt");
    if (!log.is_open()) {
        std::cerr << "Failed to open csopesy-log.txt for writing.\n";
        return;
    }

    // System Summary (like printSystemSummary)
    int totalCores = scheduler->getTotalCoreCount();
    int busy = 0;

    for (int core = 0; core < totalCores; ++core) {
        auto process = scheduler->getProcessOnCore(core);
        if (process && memoryManager->getProcessUsedMemory(process->getProcessNo()) > 0) {
            busy++;
        }
    }

    int available = totalCores - busy;
    int utilization = (static_cast<double>(busy) / totalCores) * 100;

    log << "========== System Summary ============\n";
    log << "CPU Utilization: " << utilization << "%\n";
    log << "Cores Used: " << busy << "\n";
    log << "Cores available: " << available << "\n";
    log << "======================================\n";

    // Process listing (like consolePanel.listProcesses)
    std::vector<std::shared_ptr<Process>> trulyRunning;
    for (const auto& proc : scheduler->getRunningProcesses()) {
        if (memoryManager->getProcessUsedMemory(proc->getProcessNo()) > 0) {
            trulyRunning.push_back(proc);
        }
    }

    std::unordered_set<std::shared_ptr<Process>> runningSet(trulyRunning.begin(), trulyRunning.end());

    log << "Running Processes:\n";
    for (const auto& proc : trulyRunning) {
        auto snapshot = proc->getAtomicSnapshot();
        if (snapshot.processName == "MAIN_SCREEN") continue;

        log << std::left << std::setw(15) << snapshot.processName
            << snapshot.time << "   "
            << "Core: " << snapshot.coreNo << "   "
            << snapshot.completedCommands
            << " / "
            << snapshot.totalNoCommands
            << "\n";
    }

    log << "\nFinished Processes:\n";
    int count = 0;
    for (const auto& proc : allProcesses) {
        if (proc->getProcessName() == "MAIN_SCREEN") continue;

        if (proc->isFinished() && !runningSet.count(proc)) {
            log << std::left << std::setw(15) << proc->getProcessName()
                << proc->getTime() << "   "
                << "Finished!" << "   "
                << proc->getCompletedCommands() << " / "
                << proc->getTotalNoOfCommands()
                << "\n";
            count++;
        }
    }

    if (count == 0) {
        log << "\nNo finished processes.\n";
    } else {
        log << "\nTotal finished processes: " << count << "\n";
    }

    log << "======================================\n\n";

    log.close();
    setColor(0x02); //color green
    cout << "Report generated at: " << logPath << "!\n\n";
    setColor(0x07); //default
}

void printSystemSummary(Scheduler* scheduler, std::shared_ptr<MemoryManager> memManager) {
    int totalCores = scheduler->getTotalCoreCount();
    int busy = 0;

    for (int core = 0; core < totalCores; ++core) {
        auto process = scheduler->getProcessOnCore(core);
        if (process && memManager->getProcessUsedMemory(process->getProcessNo()) > 0) {
            busy++;
        }
    }

    int available = totalCores - busy;
    int utilization = (static_cast<double>(busy) / totalCores) * 100;

    std::cout << "========== System Summary ============\n";
    std::cout << "CPU Utilization: " << utilization << "%\n";
    std::cout << "Cores Used: " << busy << "\n";
    std::cout << "Cores available: " << available << "\n";
    std::cout << "======================================\n";
}


void printHelpMenu() {
    cout << "\n";
    cout << "+----------------------------------------------------+\n";
    cout << BLUE << "|                    HELP MENU                       |\n" << RESET;
    cout << "+----------------------------------------------------+\n";
    cout << "  initialize                       - Initialize system\n";
    cout << "  screen -s <name>                 - Create process\n";
    cout << "  screen -r <name>                 - Resume existing process\n";
    cout << "  screen -c <name> <mem_size>      - Create with user-defined instructions\n";
    cout << "         \"<instructions>\"\n";
    cout << "  screen -ls                       - List all screen processes\n";
    cout << "  scheduler-start                  - Run scheduler start\n";
    cout << "  scheduler-stop                   - Stop scheduler\n";
    cout << "  report-util                      - Display utilization report\n";
    cout << "  process-smi                      - Display memory usage summary per process\n";
    cout << "  vmstat                           - Display detailed system memory and process statistics\n";
    cout << "  clear                            - Clear the screen\n";
    cout << "  help                             - Show this help menu\n";
    cout << "  exit                             - Exit the program\n\n";

    cout << "+----------------------------------------------------+\n";
    cout << BLUE << "|  Additional commands for debugging                 |\n" << RESET;
    cout << "+----------------------------------------------------+\n";
    cout << "  print-ready                      - Print current ready queue\n\n";

}

void handleExit() {
    exit(0);
}

void clear() {
	cout << "\033c" << flush;
	header();
}

void clearToProcessScreen() {
	cout << "\033c" << flush;
}

void startBatchGeneration(std::vector<std::shared_ptr<Process>>& processList, ConsolePanel& consolePanel) {
    if (isBatchGenerating) {
        std::cout << "Batch generation already running!\n\n";
        return;
    }

    isBatchGenerating = true;

    batchGeneratorThread = std::thread([&processList, &consolePanel]() {
        int localTicks = 0;

        while (isBatchGenerating) {
            // std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 1 tick = 1 ms
            localTicks++;

            if (localTicks  >= config.batchProcessFreq) {
                localTicks = 0;

                // Generate process name
                std::ostringstream ss;
                ss << "p" << std::setw(2) << std::setfill('0') << processCounter++;
                std::string procName = ss.str();

                // Random instruction count
                unsigned long long total = config.minInstructions + rand() % (config.maxInstructions - config.minInstructions + 1);

                // Generate random memory size M between min-mem-per-proc and max-mem-per-proc
                // unsigned long long memSize = config.minMemPerProcess + rand() % (config.maxMemPerProcess - config.minMemPerProcess + 1);

                unsigned long long memSize = config.minMemPerProcess;
                std::vector<unsigned long long> validSizes;
                for (unsigned long long size = config.minMemPerProcess; size <= config.maxMemPerProcess; size <<= 1) {
                    validSizes.push_back(size);
                }

                if (!validSizes.empty()) {
                    memSize = validSizes[rand() % validSizes.size()];
                }

                // new process creation to support MO2
                auto newProc = std::make_shared<Process>(procName, total, memSize, memoryManager);
                
                // Previous code was:
                // auto newProc = std::make_shared<Process>(procName, total);

                // Generate random instructions
                auto instructions = generateRandomInstructions(total, procName, memSize, config);
                for (const auto& instr : instructions)
                    newProc->addInstruction(instr);

                processList.push_back(newProc);
                
                // Create console screen
                int dummyCurr = rand() % 100;
                auto procConsole = std::make_shared<Console>(procName, dummyCurr, total, newProc->getProcessNo());
                consolePanel.addConsolePanel(procConsole);

                scheduler->addProcess(newProc);
                batchProcessCount++;
            }
            
            // check frequently even if batchProcessFreq is high
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    std::cout << "Started batch process generation.\n\n";
}

void stopBatchGeneration() {
    if (!isBatchGenerating) {
        std::cout << "No batch generation is running.\n\n";
        return;
    }

    isBatchGenerating = false;

    if (batchGeneratorThread.joinable())
        batchGeneratorThread.join();

    std::cout << "Stopped batch process generation.\n";
    std::cout << "Total processes generated: " << batchProcessCount << "\n\n";
}