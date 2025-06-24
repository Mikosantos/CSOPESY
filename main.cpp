/* Header Files */
#include "Console.h"
#include "ConsolePanel.h"
#include "Process.h"
#include "Config.h"
#include "Scheduler.h"
#include "InstructionUtils.h"
#include "FCFSScheduler.h"
#include "RRScheduler.h"

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
#include <filesystem>
#include <fstream>
#include <chrono>

#define ORANGE "\033[38;5;208m"
#define RESET  "\033[0m"

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
void report_util(const std::vector<std::shared_ptr<Process>>& processList);
void printSystemSummary();
void printHelpMenu();
void handleExit();
void clear();
void clearToProcessScreen();
void displayProcessScreen(const std::shared_ptr<Process>& proc);
void printLastUpdated();
void startBatchGeneration(std::vector<std::shared_ptr<Process>>&, ConsolePanel&);
void stopBatchGeneration();

std::unique_ptr<Scheduler> scheduler;
Config config;

std::atomic<bool> isBatchGenerating = false;
std::thread batchGeneratorThread;
std::atomic<int> batchProcessCount = 0;
int processCounter = 1;


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
        report_util(processList);
    } 
    
    else if (cmd == "screen" && args.size() == 1 && args[0] == "-ls") {
        printSystemSummary();
        consolePanel.listProcesses(processList);
    } 
    
    else if (cmd == "screen" && args.size() >= 2 && args[0] == "-s") {
        string procName = args[1];

        for (const auto& c : screens) {
            if (c->getConsoleName() == procName) {
                cout << "Process '" << procName << "' already exists. Use -r to resume.\n\n";
                return;
            }
        }

        unsigned long long total = config.minInstructions + rand() % (config.maxInstructions - config.minInstructions + 1);

        clearToProcessScreen();
        auto newProc = make_shared<Process>(procName, total);

        auto instructions = generateRandomInstructions(total);
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

        if (!foundScreen || !foundProcess || targetProcess->isFinished()) {
            cout << "Process '" << procName << "' not found.\n\n";
            return;
        }

        clearToProcessScreen();
        consolePanel.setCurrentScreen(currentPanel);
        displayProcessScreen(targetProcess);

    } 
    
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
    std::cout << "  Scheduler type     : " << ORANGE << config.schedulerType    << RESET << "\n";
    std::cout << "  Number of CPUs     : " << ORANGE << config.numCPUs          << RESET << "\n";
    std::cout << "  Quantum cycles     : " << ORANGE << config.quantumCycles    << RESET << "\n";
    std::cout << "  Batch process freq : " << ORANGE << config.batchProcessFreq << RESET << "\n";
    std::cout << "  Min instructions   : " << ORANGE << config.minInstructions  << RESET << "\n";
    std::cout << "  Max instructions   : " << ORANGE << config.maxInstructions  << RESET << "\n";
    std::cout << "  Delay per exec     : " << ORANGE << config.delaysPerExec    << RESET << "\n";

    std::cout << "\nStarting scheduler...\n";

    if (config.schedulerType == "fcfs") {
        scheduler = std::make_unique<FCFSScheduler>(config.numCPUs, config.delaysPerExec);
        scheduler->start();
        std::cout << ORANGE << "[FCFS Scheduler started with "
                  << config.numCPUs << " cores]" << RESET << "\n\n";
    } 
    
    else if (config.schedulerType == "rr") {
        scheduler = std::make_unique<RRScheduler>(config.numCPUs, config.delaysPerExec, config.quantumCycles);
        scheduler->start();
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

void report_util(const std::vector<std::shared_ptr<Process>>& processList) {
    std::filesystem::path logPath = std::filesystem::current_path() / "csopesy-log.txt";
    std::ofstream log("csopesy-log.txt");
    if (!log.is_open()) {
        std::cerr << "Failed to open csopesy-log.txt for writing.\n";
        return;
    }

    int busy = scheduler->getBusyCoreCount();
    int total = scheduler->getAvailableCoreCount() + busy;
    int utilization = (static_cast<double>(busy) / total) * 100;

    log << "========== System Summary ============\n";
    if (scheduler) {
        log << "CPU Utilization: " << utilization << "%\n";
        log << "Cores Used: " << scheduler->getBusyCoreCount() << "\n";
        log << "Cores available: " << scheduler->getAvailableCoreCount() << "\n";
    } else {
        log << "Scheduler not running.\n";
    }
    log << "======================================\n";

    // Running processes
    log << "Running Processes:\n";
    for (const auto& proc : processList) {
        if (proc->getProcessName() == "MAIN_SCREEN") continue;
        if (!proc->isFinished()  && proc->getCompletedCommands() > 0) {
            log << std::left << std::setw(15) << proc->getProcessName()
                << proc->getRawTime() << "   "
                << "Core: " << proc->getCoreNo() << "   "
                << proc->getCompletedCommands() << " / "
                << proc->getTotalNoOfCommands() << "\n";
        }
    }

    // Finished processes
    log << "\nFinished Processes:\n";
    for (const auto& proc : processList) {
        if (proc->getProcessName() == "MAIN_SCREEN") continue;
        if (proc->isFinished()) {
            log << std::left << std::setw(15) << proc->getProcessName()
                << proc->getRawTime() << "   "
                << "Finished!" << "   "
                << proc->getCompletedCommands() << " / "
                << proc->getTotalNoOfCommands() << "\n";
        }
    }
    log << "======================================\n";

    log.close();
    setColor(0x02); //color green
    cout << "Report generated at: " << logPath << "!\n\n";
    setColor(0x07); //default
}

void printSystemSummary() {
    int busy = scheduler->getBusyCoreCount();
    int total = scheduler->getAvailableCoreCount() + busy;
    int utilization = (static_cast<double>(busy) / total) * 100;

    cout << "========== System Summary ============\n";
    cout << "CPU Utilization: "    << utilization << "%\n";
    cout << "Cores Used: "         << scheduler->getBusyCoreCount() << "\n";
    cout << "Cores available: "    << scheduler->getAvailableCoreCount() << "\n";
    cout << "======================================\n";
}

void printHelpMenu() {
    cout << "  initialize        - Initialize system\n";
    cout << "  screen -s <name>  - Start new screen\n";
    cout << "  screen -r <name>  - Resume existing screen\n";
    cout << "  scheduler-start   - Run scheduler start\n";
    cout << "  scheduler-stop    - Stop scheduler\n";
    cout << "  report-util       - Display utilization report\n";
    cout << "  clear             - Clear the screen\n";
    cout << "  screen -ls        - List all screen processes\n";
    cout << "  help              - Show this help menu\n";
    cout << "  exit              - Exit the program\n\n";
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
                auto newProc = std::make_shared<Process>(procName, total);

                // Generate random instructions
                auto instructions = generateRandomInstructions(total);
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
