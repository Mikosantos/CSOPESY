@echo off
g++ -std=c++17 main.cpp Console.cpp ConsolePanel.cpp Process.cpp -o commandline_sim.exe
commandline_sim.exe
pause