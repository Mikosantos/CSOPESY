### Test 6 description:
Perform the following changes first to your emulator prior to recording:
1. A process created will always have a variable "x, y, z" in its symbol table with a value of 0.
2. All processes created will have the EXACT instruction set below generated via [min-ins, max-ins]:  
• FOR([ADD (x, x, 1), PRINT("Value from: "+x), ADD (y. y. 1), PRINT("Value from: "+y), ADD (z, z, 1), PRINT("Value from: "+z)], 100)

The parameters for your "config.txt" should be:  
num-cpu 1  
scheduler "rr"  
quantum-cycles 20  
batch-process-freq 1     
min-ins 1000  
max-ins 1000  
delay-per-exec 0  

Expected output: For all three processes viewed, variables x,y,z should clearly show an increasing value.

- Added a new function to manage predefined instruction sequences.

Included a guide detailing the syntax for setting each instruction type (DECLARE, ADD, SUBTRACT, PRINT, SLEEP, FOR).
[Instruction Guide.txt](https://github.com/user-attachments/files/20851871/Instruction.Guide.txt)


Note: No need to push this to main (test case only)
