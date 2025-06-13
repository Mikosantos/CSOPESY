#pragma once 
#include "Console.h"
#include "Process.h"

#include <string>
#include <vector>
#include <memory>

class ConsolePanel{
    private:
        static std::shared_ptr<Console> curPanel;
        static std::vector<std::shared_ptr<Console>> consolePanels;

    public:
        ConsolePanel();

        //getters
        std::string getCurrentScreenName();
        std::shared_ptr<Console> getCurrentScreen();
        std::vector<std::shared_ptr<Console>> getConsolePanels();

        //setters
        void setCurrentScreen(std::shared_ptr<Console> screenPanel);
        
        void addConsolePanel(std::shared_ptr<Console> screenPanel);
        static void listProcesses(const std::vector<std::shared_ptr<Process>>& processes);
};