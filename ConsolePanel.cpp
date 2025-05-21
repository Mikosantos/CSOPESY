#include "ConsolePanel.h"
#include "Console.h"

#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <random>

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

void ConsolePanel::listAvailableScreens() {

    std::cout << "========== System Summary ============\n";
    std::cout << "CPU Utilization: "    << 100 << "%\n";
    std::cout << "Cores Used: "         << 16 << "\n";
    std::cout << "Cores available: "    << 0 << "\n";
    std::cout << "======================================\n";
    std::cout << "Running Processes: \n\n";
    std::cout << "Finished Processes: \n";
    std::cout << "======================================\n";
    std::cout << "Available Screens:\n";

    for (const auto& consolePtr : ConsolePanel::consolePanels) {
        std::cout << consolePtr->getConsoleName()
                  << " - Created at: " << consolePtr->getCreationTime() << "\n";
    }

    std::cout << "\n";

    // std::cout << "\nCurrently Selected Console: ";
    // if (ConsolePanel::curPanel != nullptr) {
    //     std::cout << ConsolePanel::curPanel->getConsoleName() << "\n\n";
    // } else {
    //     std::cout << "None selected\n\n";
    // }
}

void ConsolePanel::addConsolePanel(std::shared_ptr<Console> screenPanel){
    consolePanels.push_back(screenPanel);
}
