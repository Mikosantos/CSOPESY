/* Header Files */
#include "Console.h"
#include "ConsolePanel.h"
#include "Process.h"
#include "Scheduler.h"
#include "Config.h"

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

void handleMainScreenCommands(const string& cmd, const vector<string>& args, ConsolePanel& consolePanel, vector<shared_ptr<Process>>& processList, 
                              bool& hasInitialized, bool& notShuttingDown);
void handleProcessScreenCommands(const string& cmd, const string& currentScreenName, const vector<shared_ptr<Process>>& processList, ConsolePanel& consolePanel);
void setColor(unsigned char color);
void header();
pair<string, vector<string>> parseCommand(const string& input);
void initialize();
void scheduler_start();
void scheduler_stop();
void report_util(const std::vector<std::shared_ptr<Process>>& processList);
void printSystemSummary();
void printHelpMenu();
void handleExit();
void clear();
void clearToProcessScreen();
void displayProcessScreen(shared_ptr<Process> nProcess);

std::unique_ptr<Scheduler> scheduler;
Config config;

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
        scheduler_start();
    } 
    
    else if (cmd == "scheduler-stop") {
        scheduler_stop();
    } 
    
    else if (cmd == "report-util") {
        report_util(processList);
    } 
    
    else if (cmd == "screen" && args.size() >= 1 && args[0] == "-ls") {
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

        int curr = 1 + rand() % 100;
        int total = config.minInstructions + rand() % (config.maxInstructions - config.minInstructions + 1);
        
        // what's the difference between total and cmds?
        int cmds = total + rand() % 100;

        clearToProcessScreen();
        auto newProc = make_shared<Process>(procName, total);
        processList.push_back(newProc);

        scheduler->addProcess(newProc);

        // di ko pa gets
        auto procConsole = make_shared<Console>(procName, curr, total, newProc->getProcessNo());
        consolePanel.addConsolePanel(procConsole);
        consolePanel.setCurrentScreen(procConsole);

        displayProcessScreen(newProc);

    } 
    
    else if (cmd == "screen" && args.size() >= 2 && args[0] == "-r") {
        string procName = args[1];
        bool foundScreen = false, foundProcess = false;
        std::shared_ptr<Process> targetProcess = nullptr;

        for (auto& s : screens) {
            if (s->getConsoleName() == procName) {
                foundScreen = true;
                consolePanel.setCurrentScreen(s);
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
        // cout << "Finished!\n\n";
    } 
    
    else {
        cout << "Only 'exit' and 'process-smi' commands are allowed inside a process screen.\n\n";
    }
}

void displayProcessScreen(shared_ptr<Process> newProcess) {
    cout << "\n=====================================================\n";
    cout << "                  PROCESS CONSOLE SCREEN             \n";
    cout << "=====================================================\n";
    cout << "Process name: " << newProcess->getProcessName() << "\n";
    cout << "ID: " << ORANGE << newProcess->getProcessNo() << RESET << "\n";
    cout << "Logs: "<< endl;
    cout << newProcess->getTime() << " Core: " << ORANGE << newProcess->getCoreNo() << RESET << " \"Hello world from " << newProcess->getProcessName() << "!\""<< "\n\n";
    cout << "Current instruction line: " << ORANGE << newProcess->getCompletedCommands() << RESET << "\n";
    cout << "Lines of instruction: "     << ORANGE << newProcess->getTotalNoOfCommands() << RESET << "\n";
    cout << "=====================================================\n";
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
        scheduler = std::make_unique<Scheduler>(config.numCPUs, config.delaysPerExec);
        scheduler->start();
        std::cout << ORANGE << "[FCFS Scheduler started with "
                  << config.numCPUs << " cores]" << RESET << "\n\n";
    } else if (config.schedulerType == "rr") {
        std::cout << "Round Robin scheduler is not yet implemented.\n\n";
    } else {
        std::cout << "Invalid scheduler type in config file.\n\n";
        return;
    }
}

// creates X number of processes with random instruction lines
void scheduler_start() {
	cout << "'scheduler-start' command recognized. Doing something.\n\n";
}

// stops generating dummy processes
void scheduler_stop() {
	cout << "'scheduler-stop' command recognized. Doing something.\n\n";
}

void report_util(const std::vector<std::shared_ptr<Process>>& processList) {
    std::ofstream log("csopesy-log.txt");
    if (!log.is_open()) {
        std::cerr << "Failed to open csopesy-log.txt for writing.\n";
        return;
    }

    log << "========== System Summary ============\n";
    if (scheduler) {
        log << "CPU Utilization: " << "100%" << "\n";  // Placeholder
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
        if (!proc->isFinished()) {
            log << std::left << std::setw(15) << proc->getProcessName()
                << proc->getRawTime() << "   "
                << "Core: " << proc->getCoreNo() << "   "
                << proc->getCompletedCommands() << " / "
                << proc->getTotalNoOfCommands() << "\n";
        }
    }

    // Finished processes
    log << "\nFinished Processes:\n";
    log << "======================================\n";
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
    cout << "System utilization report saved to csopesy-log.txt\n\n";
}

void printSystemSummary() {
    cout << "========== System Summary ============\n";
    cout << "CPU Utilization: "    << 100 << "%\n";
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