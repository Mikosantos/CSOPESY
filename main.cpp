#include <iostream>
#include <windows.h>
#include <map>
#include <sstream>

using namespace std;

struct ScreenData {
	string processName;
	int currentLine;
	int totalLines;
	string timestamp;
};

bool inScreen = false;

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

void screen() {
	cout << "'screen' command recognized. Doing something.\n";
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


pair<string, string> parseCommand(const string& input) {
	istringstream stream(input);
	string cmd, argument;
	stream >> cmd;
	getline(stream, argument);
	if (!argument.empty() && argument[0] == ' ') {
	argument = argument.substr(1);
	}
	return {cmd, argument};
}

int main() {
	header();
	string input;
	
	while (true) {
		cout << "Enter a command: ";
		getline(cin, input);
		
		pair<string, string> parsedCommand = parseCommand(input);
		string cmd = parsedCommand.first;
		string argument = parsedCommand.second;
		
		if (cmd == "initialize"  && inScreen == false) {
			initialize();
		} else if (cmd == "screen" && inScreen == false) {
			screen();
		} else if (cmd == "scheduler-test" && inScreen == false) {
			scheduler_test();
		} else if (cmd == "scheduler-stop" && inScreen == false) {
			scheduler_stop();
		} else if (cmd == "report-util" && inScreen == false) {
			report_util();
		} else if (cmd == "clear" && inScreen == false) {
			clear();
		} else if (cmd == "exit") {
			exit();
		} else cout << "Command not recognized.\n";
	}
}