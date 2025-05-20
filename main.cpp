/* Header Files */
#include "Console.h"
#include "ConsolePanel.h"
#include "Process.h"

/* Libraries */
#include <string>
#include <iostream>
#include <random>
#include <windows.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <sstream>

using namespace std;

void setColor(unsigned char color);
void header();
pair<string, vector<string>> parseCommand(const string& input);
void initialize();
void scheduler_start();
void scheduler_stop();
void report_util();
void printHelpMenu();
void handleExit();
void clear();
void clearToProcessScreen();
void displayProcessScreen(shared_ptr<Process> nProcess);

int main() {

    srand(static_cast<unsigned>(time(nullptr)));

    string input;
    ConsolePanel consolePanel;
    bool notShuttingDown = true;
    int countProcesses = 0;
    string screenProcessName = "";
    bool hasInitialized = false;
    pair<string, vector<string>> parsedCommand;
    int currentLine, totalLine, totalCommand;
    vector<shared_ptr<Process>> processList;

    header();

    while (notShuttingDown) {
        cout << "root:\\> ";
        getline(cin, input);
        parsedCommand = parseCommand(input);
        string  cmd = parsedCommand.first;
        vector<string> args = parsedCommand.second;

        if (cmd != "initialize" && cmd != "exit" && !hasInitialized) {
            cout << "Initialize the program with command \"initialize\" first!" << "\n\n";
            continue;
        }

        /* Screen and command checkers
        
        cout << "=====================================================\n";
        cout << "ConsoleName: " << consolePanel.getCurrentScreenName() << endl;
        cout << "cmd:         " << cmd << endl;
        if (args.size() >= 2){
            cout << "argSize      " << args.size() << endl;
            cout << "procOpt      " << args[0] << endl;
            cout << "procName:    " << args[1] << endl;
        }
        consolePanel.listAvailableScreens();
        cout << "=====================================================\n";
        */

        vector<shared_ptr<Console>> screens = consolePanel.getConsolePanels();

        // exit command
        if ((consolePanel.getCurrentScreenName() != "MAIN_SCREEN") && cmd == "exit") {
            system("cls");
            for (auto& screenPtr : screens) {
                if (screenPtr->getConsoleName() == "MAIN_SCREEN") {
                    consolePanel.setCurrentScreen(screenPtr);
                    break;
                }
            }
            if (consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
                clear();
            }
        } else if ((consolePanel.getCurrentScreenName() == "MAIN_SCREEN") && cmd == "exit") {
            notShuttingDown = false;
            handleExit();
        }

        // The -s and -r commands won't work when not on the main screen
        else if (cmd == "screen" && args.size() >= 2 && (args[0] == "-s" || args[0] == "-r") && consolePanel.getCurrentScreenName() != "MAIN_SCREEN") {
            cout << "Cannot create or resume a screen while inside another screen. Type 'exit' to return first.\n\n";
        }

        // -s command
        else if (cmd == "screen" && args.size() >= 2 && args[0] == "-s") {
            bool exists = false;
            for (auto& consolePtr : screens) {
                if (consolePtr->getConsoleName() == args[1]) {
                    cout << "Process '" << args[1] << "' already exists. Use -r to resume.\n\n";
                    exists = true;
                    break;
                }
            }
            if(exists) continue;

            currentLine = 1 + rand() % 100;
            totalLine = currentLine + rand() % 100;
            totalCommand = totalLine + rand() % 100;
        
            clearToProcessScreen();
            shared_ptr<Process>  newProcess = make_shared<Process>(args[1], totalCommand);
            processList.push_back(newProcess);

            /* Process Screen */
            shared_ptr<Console> newProcessConsole = make_shared<Console>(args[1], currentLine, totalLine, newProcess->getProcessNo());
            consolePanel.addConsolePanel(newProcessConsole);
            consolePanel.setCurrentScreen(newProcessConsole);
            
            displayProcessScreen(newProcess);
            // cout << "Finished!" << "\n\n\n";
        }
        
        // -r command (checking for existing screen)
        else if (cmd == "screen" && args.size() >= 2 && args[0] == "-r") {
            screenProcessName = args[1];

            // Check if the screen and process exist first
            bool screenFound = false;
            bool processFound = false;

            for (auto& consolePtr : screens) {
                if (consolePtr->getConsoleName() == screenProcessName) {
                    screenFound = true;
                    consolePanel.setCurrentScreen(consolePtr);
                    break;
                }
            }

            for (auto& processPtr : processList) {
                if (processPtr->getProcessName() == screenProcessName) {
                    processFound = true;
                    break;
                }
            }

            if (!screenFound || !processFound) {
                cout << "No such screen '" << screenProcessName << "' found.\n\n";
            } else {
                // Everything is valid — now you can clear and display the screen
                clearToProcessScreen();
                for (auto& processPtr : processList) {
                    if (processPtr->getProcessName() == screenProcessName) {
                        displayProcessScreen(processPtr);
                        cout << "\n\n";
                        break;
                    }
                }
            }
        } 

        // else if ((args.size() >= 2 && ((args[0] == "-s" && !args[1].empty()) || (args[0] == "-r" && !args[1].empty()))) && (consolePanel.getCurrentScreenName() != "MAIN_SCREEN")) {
        //     cout << "Command not recognized.\n\n";
        // } 

        // initialize command
        else if (cmd == "initialize" && !hasInitialized && consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
            initialize();
            hasInitialized = true;
        } else if (cmd == "initialize" && hasInitialized && consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
            cout << "System has already been initialized. Proceed with other commands.\n\n";
        } 
        
        // other commands
        else if (cmd == "scheduler-start" && consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
            scheduler_start();
        } else if (cmd == "scheduler-stop" && consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
            scheduler_stop();
        } else if (cmd == "report-util" && consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
            report_util();
        } else if (cmd == "help" && consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
            printHelpMenu();
        } 
        
        // list all processes (di pa okay)
        else if (cmd == "screen" && args.size() >= 1 && args[0] == "-ls" && consolePanel.getCurrentScreenName() == "MAIN_SCREEN") {
            consolePanel.listAvailableScreens();
        } 
        
        else if (cmd == "clear" && consolePanel.getCurrentScreenName() == "MAIN_SCREEN"){
            clear();
        } 
        
        else if (cmd == "process-smi" && consolePanel.getCurrentScreenName() != "MAIN_SCREEN"){
            screenProcessName = consolePanel.getCurrentScreenName();
            for (auto& processPtr: processList){
                if(processPtr->getProcessName() == screenProcessName){
                    displayProcessScreen(processPtr);
                    break;
                }
            }
            cout << "Finished!" << "\n\n";
        } 
        
        // error command/s
        else {
            cout << "Unknown command! Type \"help\" for commandlist." << "\n\n";
        }
    }
}

void displayProcessScreen(shared_ptr<Process> newProcess) {
    cout << "\n=====================================================\n";
    cout << "                  PROCESS CONSOLE SCREEN             \n";
    cout << "=====================================================\n";
    cout << "Process name: " << newProcess->getProcessName() << "\n";
    cout << "ID: " << newProcess->getProcessNo() << "\n";
    cout << "Logs: "<< endl;
    cout << "(" << newProcess->getTime() << ") " << "Core:" <<newProcess->getCoreNo() << " \"Hello world from " << newProcess->getProcessName() << "!\""<< "\n\n";
    cout << "Current instruction line: " << newProcess->getCompletedCommands() << "\n";
    cout << "Lines of code: " << newProcess->getTotalNoOfCommands() << "\n";
    
    cout << "=====================================================\n";
}

void setColor( unsigned char color ){
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), color );
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

    cout << "Last updated: 05-20-25\n\n";

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
	cout << "'initialize' command recognized. Doing something.\n\n";
}

void scheduler_start() {
	cout << "'scheduler-start' command recognized. Doing something.\n\n";
}

void scheduler_stop() {
	cout << "'scheduler-stop' command recognized. Doing something.\n\n";
}

void report_util() {
	cout << "'report-util' command recognized. Doing something.\n\n";
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
	system("cls"); 
	header();
}

void clearToProcessScreen() {
	system("cls"); 
}