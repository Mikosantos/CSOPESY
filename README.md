# OS Emulator – Process Multiplexer & Scheduler (FCFS & RR)

## Developers
- @Albarracin, Clarissa  
- @Garcia, Reina Althea  
- @Santos, Miko  

## Project Description
This project is the **first part** of an Operating System emulator. It includes the implementation of a **process multiplexer** and a **command-line interpreter (CLI)** that allows you to:

- **Create**, **redraw**, and **list** screen-based processes  
- **Start** and **stop** the scheduler to stress test the system  
- Configure the scheduler via a `config.txt` file  
- Use two types of scheduling algorithms:  
  - **First-Come, First-Served (FCFS)**  
  - **Round Robin (RR)**

## Features
- CLI-based interaction
- Simulated screen processes
- Process scheduling (FCFS and RR)
- Configuration via `config.txt`
- Stress testing through CLI commands

## Configuration
To set up the program:

1. Edit the `config.txt` file to configure your scheduling preferences.
2. Use the `initialize` command in the CLI to apply the configuration.

## Compilation & Running
To compile the program using **g++** with **C++20** support, run the following command in the terminal or command prompt:

```bash
g++ -std=c++20 main.cpp Console.cpp ConsolePanel.cpp Process.cpp Scheduler.cpp Config.cpp FCFSScheduler.cpp RRScheduler.cpp FlatMemoryAllocator.cpp MemoryAllocator.cpp -o main.exe
```
To run the program:
```bash
main.exe
```
