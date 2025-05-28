#include "ConsolePanel.h"
#include "Console.h"

#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <random>
#include <iomanip>

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


//getters
std::shared_ptr<Console> ConsolePanel::getCurrentScreen(){
    return ConsolePanel::curPanel;
}
std::string ConsolePanel::getCurrentScreenName(){
    return ConsolePanel::curPanel->getConsoleName();
}
std::vector<std::shared_ptr<Console>> ConsolePanel::getConsolePanels(){
    return ConsolePanel::consolePanels;
}

//setters
void ConsolePanel::setCurrentScreen(std::shared_ptr<Console> screenPanel){
    ConsolePanel::curPanel = screenPanel;
}

void ConsolePanel::listProcesses(const std::vector<std::shared_ptr<Process>>& processes) {
    std::cout << "Running Processes: \n";

    for (const auto& proc : processes) {
        if (proc->getProcessName() == "MAIN_SCREEN") continue;

        std::cout << std::left << std::setw(15) << proc->getProcessName()
                  << " (" << proc->getTime() << ")   "
                  << "Core:" << proc->getCoreNo() << "   "
                  << proc->getCompletedCommands() << "/" << proc->getTotalNoOfCommands()
                  << "\n";
    }

    std::cout << "\nFinished Processes: \n";
    std::cout << "======================================\n";
}

void ConsolePanel::addConsolePanel(std::shared_ptr<Console> screenPanel){
    consolePanels.push_back(screenPanel);
}
