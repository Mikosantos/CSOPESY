Note: to run **"RoundRobin"** branch using run/debug button in VSCode, change args in tasks.json to the following:

```bash
"args": [
  "-std=c++20",
  "${file}",
  "${workspaceFolder}/Console.cpp",
  "${workspaceFolder}/ConsolePanel.cpp",
  "${workspaceFolder}/Process.cpp",
  "${workspaceFolder}/scheduler.cpp",
  "${workspaceFolder}/Config.cpp",
  "${workspaceFolder}/FCFSScheduler.cpp",
  "${workspaceFolder}/RRScheduler.cpp",
  "-o",
  "${fileDirname}\\${fileBasenameNoExtension}.exe"
],
