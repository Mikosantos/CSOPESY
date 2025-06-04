#include "ConsolePanel.h"
#include "Console.h"

#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <random>
#include <iomanip>

#define ORANGE "\033[38;5;208m"
#define RESET  "\033[0m"
#define BLUE   "\033[34m"

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

    //if not finished

    for (const auto& proc : processes) {
        if (proc->getProcessName() == "MAIN_SCREEN") continue;

        if(!proc->isFinished()) {
            std::cout << std::left << std::setw(15) << proc->getProcessName()
                  << proc->getTime() << "   "
                  << "Core: "   << ORANGE << proc->getCoreNo()  << RESET << "   "
                  << ORANGE     << proc->getCompletedCommands() << RESET << BLUE << " / " << RESET
                  << ORANGE     << proc->getTotalNoOfCommands() << RESET
                  << "\n";
        } 
    }

    std::cout << "\nFinished Processes: \n";

    //if finished

    for (const auto& proc : processes) {
        if (proc->getProcessName() == "MAIN_SCREEN") continue;

        if(proc->isFinished()) {
            std::cout << std::left << std::setw(15) << proc->getProcessName()
                  << proc->getTime() << "   "
                  << "Finished!"<< ORANGE << proc->getCoreNo()  << RESET << "   "
                  << ORANGE     << proc->getCompletedCommands() << RESET << BLUE << " / " << RESET
                  << ORANGE     << proc->getTotalNoOfCommands() << RESET
                  << "\n";
        } 
    }

    std::cout << "======================================\n";
}

void ConsolePanel::addConsolePanel(std::shared_ptr<Console> screenPanel){
    consolePanels.push_back(screenPanel);
}
