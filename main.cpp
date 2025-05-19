#include <iostream>
#include <windows.h>
#include <map>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdlib>   // rand, srand

using namespace std;
bool inScreen = false;

struct ScreenInfo {
    string name;
    int currentLine;
    int totalLines;
    string timestamp;
};

vector<string> processNames; // global variable to store process names for checking

void setColor( unsigned char color )
{
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), color );
}

void header() {
	setColor(0x07);
	cout << "  ____ ____  ____  _____ _____ ____ __   __     "<< endl;
	cout << " / __/  ___|/ __ `|  _  ` ____/ ___`  ` / /     "<< endl;
	cout << "| |   `___ ` |  | | |_| |  __|`___ ` `   /      "<< endl;
	cout << "| |__ ___) | |__| | ___/| |___ ___) | | |       "<< endl;
	cout << " `___` ____/`____/|_|   |_____|___ /  |_|       "<< endl;
	setColor(0x02);
	cout << "Hello, Welcome to CSOPESY commandline!\n";
	setColor(0x0E);
	cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
	setColor(0x07);
}

void initialize() {
	cout << "'initialize' command recognized. Doing something.\n";
}

string getTimestamp() {
	time_t timestamp;
	tm * tm_stamp;
	char ts_string[100];
			
	time(&timestamp);
	tm_stamp = localtime(&timestamp);
			
	strftime(ts_string, 100, "%m/%d/%Y, %I:%M:%S %p", tm_stamp);

	return ts_string;
}

bool processExists(const string& name) {
	for (string& p : processNames) {
		if (p == name) return true;
	}
	return false;
}

void displayScreen(const ScreenInfo& screen) {
	system("cls");
	cout << "==================== SCREEN: " << screen.name << " ====================\n";
	cout << "Process: " << screen.name << "\n\n";
	cout << "Current line of instruction: " << screen.currentLine << "\n";
	cout << "Total lines of instruction: " << screen.totalLines << "\n\n";
	cout << "Timestamp: " << screen.timestamp << "\n";
	cout << "=====================================================\n";
	// cout << "[ " << screen.name << " ] $ ";
}

void screen(vector<string> args, vector<ScreenInfo>& processes) {
	if (args.size() < 2) {
		cout << "Invalid screen command.\n\n";
		return;
	}

	string option = args[0];
	string process_name = args[1];

	if (option == "-s") {
		// Check if process already exists
		for (ScreenInfo& s : processes) {
			if (s.name == process_name) {
				cout << "Process '" << process_name << "' already exists. Use -r to resume.\n\n";
				return;
			}
		}

		// Create and store new screen (store in vector)
		ScreenInfo newScreen;
		newScreen.name = process_name;
		newScreen.currentLine = 1 + rand() % 100;
		newScreen.totalLines = newScreen.currentLine + rand() % 100;
		newScreen.timestamp = getTimestamp();

		processes.push_back(newScreen);
		inScreen = true;
		displayScreen(newScreen);

	} else if (option == "-r") {
		for (ScreenInfo& s : processes) {
			if (s.name == process_name) {
				inScreen = true;
				displayScreen(s);
				return;
			}
		}
		cout << "No such screen '" << process_name << "' found.\n";
	} else {
		cout << "Unknown option for screen command.\n";
	}
}

void scheduler_test() {
	cout << "'scheduler-test' command recognized. Doing something.\n";
}

void scheduler_stop() {
	cout << "'scheduler-stop' command recognized. Doing something.\n";
}

void report_util() {
	cout << "'report-util' command recognized. Doing something.\n";
}

//
void listAvailableScreens(const vector<ScreenInfo>& processes) {
    if (processes.empty()) {
        cout << "No screens available.\n\n";
        return;
    }

    // cout << "Available screens:\n";
    for (const ScreenInfo& p : processes) {
        cout << "- " << p.name << endl;
    }
	cout << "\n";
}

void printHelpMenu() {
    cout << "  initialize        - Initialize system\n";
    cout << "  screen -s <name>  - Start new screen\n";
    cout << "  screen -r <name>  - Resume existing screen\n";
    cout << "  scheduler-test    - Run scheduler test\n";
    cout << "  scheduler-stop    - Stop scheduler\n";
    cout << "  report-util       - Display utilization report\n";
    cout << "  clear             - Clear the screen\n";
    cout << "  ls                - List all screen processes\n";
    cout << "  help              - Show this help menu\n";
    cout << "  exit              - Exit the program\n";
	cout << "\n";
}
//

void clear() {
	cout << "'clear' command recognized. Doing something.\n";
	#ifdef _WIN32
	system("cls"); 
	#endif
	header();
}

void exit() {
	if (inScreen) {
        inScreen = false;
        system("cls");
        header();
    } else {
        cout << "'exit' command recognized. Exiting program.\n";
        exit(0);
    }
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

int main() {
	srand(static_cast<unsigned>(time(nullptr)));

	header();
	string input;
	vector<ScreenInfo> processes;
	
	while (true) {
		if (inScreen) {
			cout << "[ " << processes.back().name << " ] $ ";
		} else {
			cout << "Enter a command: ";
		}
		
		getline(cin, input);
		
		pair<string, vector<string>> parsedCommand = parseCommand(input);
		string cmd = parsedCommand.first;
		vector<string> args = parsedCommand.second;
		
		if (cmd == "initialize"  && inScreen == false) {
			initialize();
		} else if (cmd == "screen" && inScreen == false) {
			screen(args, processes);
		} else if (cmd == "scheduler-test" && inScreen == false) {
			scheduler_test();
		} else if (cmd == "scheduler-stop" && inScreen == false) {
			scheduler_stop();
		} else if (cmd == "report-util" && inScreen == false) {
			report_util();
		// mema add lang
		}  else if (cmd == "ls" && inScreen == false) {
			listAvailableScreens(processes);
		//
		} else if (cmd == "help" && inScreen == false) {
			printHelpMenu();
		} else if (cmd == "clear" && inScreen == false) {
			clear();
		} else if (cmd == "exit") {
			exit();
		} else cout << "Command not recognized.\n\n";
	}
}
