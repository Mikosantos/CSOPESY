#include <iostream>
#include <windows.h>
#include <map>
#include <sstream>
#include <vector>
#include <ctime>

using namespace std;
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

string getTimestamp() {
	time_t timestamp;
	tm * tm_stamp;
	char ts_string[100];
			
	time(&timestamp);
	tm_stamp = localtime(&timestamp);
			
	strftime(ts_string, 100, "%m/%d/%Y, %I:%M:%S %p", tm_stamp);

	return ts_string;
}


void screen(vector<string> args, vector<string> processes) {
	cout << "'screen' command recognized. Doing something.\n";

	if (args.size() >= 2) {
		string cmd = args[0];
		string process_name = args[1];
		
		if (cmd == "-s" && !process_name.empty()) {
			system("cls");
			cout << "Process: " << process_name << "\n\n";
			processes.push_back(process_name);

			cout << "Current line of instruction: " << 32 << "\n";
			cout << "Total lines of instruction: " << 32 << "\n\n";

			string timestamp = getTimestamp();
			cout << "Timestamp: " << timestamp << "\n";

		} else if (cmd == "-r" && !process_name.empty()) {
			system("cls");
			cout << "Process: " << process_name << "\n\n";

			// TODO: redraw console of current process

		} else {
			cout << "Invalid screen command.\n";
		}
	} else {
		cout << "No screen arguments provided.\n";
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
	header();
	string input;
	
	while (true) {
		cout << "Enter a command: ";
		getline(cin, input);
		
		pair<string, vector<string>> parsedCommand = parseCommand(input);
		string cmd = parsedCommand.first;
		vector<string> args = parsedCommand.second;

		vector<string> processes = {};
		
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
		} else if (cmd == "clear" && inScreen == false) {
			clear();
		} else if (cmd == "exit") {
			exit();
		} else cout << "Command not recognized.\n";
	}
}
