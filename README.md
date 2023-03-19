# RISC-V-Simulator
A RISC-V simulator with five pipelined stages and hazard detection and handling.

## Description
This is a RISC-V simulator with five stages:
1. Instruction Fetch
2. Instruction Decode
3. Execute
4. Memory Access
5. Write Back  
<a/>
This RISC-V simulator runs the five stages in a pipelined manner to be more efficient. However, running in a pipeline introduces data hazards, like Read-After-Write (RAW), Write-After-Read (WAR), and Write-After-Write (WAW) dependencies. This simulator has data hazard detection for true dependencies (or RAW dependencies), and it uses bypassing with a forwarding unit to pass data straight from the ALU or the memory result of the previous instruction to the current one.

## How to run
* Compile: make
* Run: ./RVSim ../cpu_traces/{RISC-V code file}
