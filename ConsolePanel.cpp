#include "ConsolePanel.h"
#include "Console.h"

#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <random>
#include <iomanip>
#include <unordered_set>

#define ORANGE "\033[38;5;208m"
#define RESET  "\033[0m"
#define BLUE   "\033[34m"
#define LIGHT_RED "\033[91m"

std::shared_ptr<Console> ConsolePanel::curPanel = nullptr;
std::vector<std::shared_ptr<Console>> ConsolePanel::consolePanels;

//Constructor
ConsolePanel::ConsolePanel(){
    bool hasMainScr = false;

    for (auto& console_pointer : consolePanels){
            if(console_pointer->getConsoleName() == "MAIN_SCREEN"){
                hasMainScr = true;
                break;
            }
    }

    if(hasMainScr == false){
        std::shared_ptr<Console> mainScreenConsole = std::make_shared<Console>("MAIN_SCREEN", 32, 32, 0);
        consolePanels.push_back(mainScreenConsole);
        if(curPanel == nullptr){
            curPanel = mainScreenConsole;
        }
    }
}


// getters -----------------------------------------------------------
std::shared_ptr<Console> ConsolePanel::getCurrentScreen(){
    return ConsolePanel::curPanel;
}

std::string ConsolePanel::getCurrentScreenName(){
    return ConsolePanel::curPanel->getConsoleName();
}

std::vector<std::shared_ptr<Console>> ConsolePanel::getConsolePanels(){
    return ConsolePanel::consolePanels;
}

// setters -----------------------------------------------------------

// This allows for the switching of the current screen to a different console panel (screen process)
// This is used when the user wants to view a specific process screen (screen -r <process_name>).
void ConsolePanel::setCurrentScreen(std::shared_ptr<Console> screenPanel){
    ConsolePanel::curPanel = screenPanel;
}

/* This displays a summary of both running and finished processes in the system.

   Each process's data is accessed using a thread-safe snapshot via 'getAtomicSnapshot()' to
   prevent data races during display.

   To avoid duplication, it uses a set of currently running processes ('runningProcesses')
   and excludes them from the "Finished" list even if marked finished, ensuring accurate display.
 */
void ConsolePanel::listProcesses(const std::vector<std::shared_ptr<Process>>& allProcesses,
                                 const std::vector<std::shared_ptr<Process>>& runningProcesses) {
    std::unordered_set<std::shared_ptr<Process>> runningSet(runningProcesses.begin(), runningProcesses.end());

    std::cout << "Running Processes:\n";
    for (const auto& proc : runningProcesses) {
        auto snapshot = proc->getAtomicSnapshot();
        if (snapshot.processName == "MAIN_SCREEN") continue;

        std::cout << std::left << std::setw(15) << snapshot.processName
                  << snapshot.time << "   "
                  << "Core: " << ORANGE << snapshot.coreNo << RESET << "   "
                  << ORANGE << snapshot.completedCommands << RESET
                  << BLUE << " / " << RESET
                  << ORANGE << snapshot.totalNoCommands << RESET
                  << "\n";
    }

    std::cout << "\nFinished Processes:\n";
    int count = 0;
    for (const auto& proc : allProcesses) {
        if (proc->getProcessName() == "MAIN_SCREEN") continue;

        if (proc->isFinished() && !runningSet.count(proc)) {
            std::cout << std::left << std::setw(15) << proc->getProcessName()
                      << proc->getTime()                            << "   "
                      << "Finished!"                                << RESET << "   "
                      << ORANGE     << proc->getCompletedCommands() << RESET << BLUE << " / " << RESET
                      << ORANGE     << proc->getTotalNoOfCommands() << RESET
                      << "\n";
            count++;
        }
    }

    if (count == 0) {
        std::cout << LIGHT_RED << "\nNo finished processes.\n" << RESET;
    } else {
        std::cout << "\nTotal finished processes: " << ORANGE << count << RESET << "\n";
    }

    std::cout << "======================================\n\n";
}



// This function adds a new console panel (screen) to the list of console panels.
void ConsolePanel::addConsolePanel(std::shared_ptr<Console> screenPanel){
    consolePanels.push_back(screenPanel);
}

void ConsolePanel::listMemoryUsageOfRunningProcesses(
    const std::vector<std::shared_ptr<Process>>& runningProcesses,
    std::shared_ptr<MemoryManager> memManager
) {
    constexpr size_t KIB = 1024;

    if (runningProcesses.empty()) {
        std::cout << LIGHT_RED << "No running processes.\n" << RESET;
        return;
    }

    std::unordered_map<int, size_t> memSnapshot = memManager->getAllProcessMemoryUsage();

    for (const auto& proc : runningProcesses) {
        auto snapshot = proc->getAtomicSnapshot();
        if (snapshot.processName == "MAIN_SCREEN") continue;

        size_t memBytes = 0;
        auto it = memSnapshot.find(snapshot.processNum);
        if (it != memSnapshot.end()) {
            memBytes = it->second;
        }

        std::cout << std::left << std::setw(20) << snapshot.processName
                  << ORANGE << memBytes << " Byte" << RESET << "\n";
    }
}
