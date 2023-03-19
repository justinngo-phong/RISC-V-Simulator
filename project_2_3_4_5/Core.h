#ifndef __CORE_H__
#define __CORE_H__

#include "Instruction_Memory.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define BOOL bool

typedef uint8_t Byte;
typedef int64_t Signal;
typedef int64_t Register;

struct Core;
typedef struct Core Core;
// Implement the following functions in Core.c
// (1). Control Unit.
typedef struct ControlSignals
{
    Signal Branch;
    Signal MemRead;
    Signal MemtoReg;
    Signal ALUOp;
    Signal MemWrite;
    Signal ALUSrc;
    Signal RegWrite;
}ControlSignals;

typedef struct Decode
{
	// Instruction fields
	Signal opcode;
	Signal rd;
	Signal funct3;
	Signal rs1;
	Signal rs2;
	Signal funct7;

	// Control signals
	ControlSignals ctrl_signals;

	// Other values
	Signal ALU_ctrl_signal;
	Signal reg1_val;
	Signal reg2_val;
	Signal immediate;
}Decode;

typedef struct Exec
{
	// ALU second value, either reg2_val or imm
	Signal ALU_2nd_val;
	
	// ALU result;
	Signal ALU_result;
	Signal zero;
	Signal neg;
}Exec;

typedef struct PipeInstr
{
	Signal instruction;
	Decode *dec;
	Exec *ex;
	Signal mem_res;
}PipeInstr;

typedef struct Core
{
    Tick clk; // Keep track of core clock
    Addr PC; // Keep track of program counter

    // What else you need? Data memory? Register file?
    Instruction_Memory *instr_mem;
   
    Byte data_mem[1024]; // data memory

    Register reg_file[32]; // register file.

    bool (*tick)(Core *core);
	PipeInstr **pipe;
}Core;

void storeDataMem(Core *core, int64_t data, int start);
int64_t loadDataMem(Core *core, int start);
void fetch(Core *core, PipeInstr *PI);
void decode(Core *core, PipeInstr *PI);
void execute(Core *core, PipeInstr *PI, int pipe_idx);
void memAccess(Core *core, PipeInstr *PI);
void writeBack(Core *core, PipeInstr *PI);

Core *initCore(Instruction_Memory *i_mem);
bool tickFunc(Core *core);

void ControlUnit(Signal input,
                 ControlSignals *signals);

// (2). ALU Control Unit.
Signal ALUControlUnit(Signal ALUOp,
                      Signal Funct7,
                      Signal Funct3);

// (3). Imme. Generator
Signal ImmeGen(Signal input);

// (4). ALU
void ALU(Signal input_0,
         Signal input_1,
         Signal ALU_ctrl_signal,
         Signal *ALU_result,
         Signal *zero,
		 Signal *neg);

// (4). MUX
Signal MUX(Signal sel,
           Signal input_0,
           Signal input_1);

// MUX3
Signal MUX3(Signal sel,
           Signal input_0,
           Signal input_1,
           Signal input_2);

// (5). Add
Signal Add(Signal input_0,
           Signal input_1);

// (6). ShiftLeft1
Signal ShiftLeft1(Signal input);

#endif
