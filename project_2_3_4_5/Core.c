#include "Core.h"
#include <inttypes.h>

Core *initCore(Instruction_Memory *i_mem)
{
	int i;

    Core *core = (Core *)malloc(sizeof(Core));
    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tickFunc;

    // initialize register file here.
    // core->data_mem[0] = ...
	for (i=0; i<1024; i++) {
		core->data_mem[i] = 0;
	}

    // initialize data memory here.
    // core->reg_file[0] = ...
	for (i=0; i<32; i++) {
		core->reg_file[i] = 0;
	}

	storeDataMem(core, -63, 40);
	storeDataMem(core, 63, 48);

	core->reg_file[1] = 0;
	core->reg_file[2] = 10;
	core->reg_file[3] = -15;
	core->reg_file[4] = 20;
	core->reg_file[5] = 30;
	core->reg_file[6] = -35;

    return core;
}

void storeDataMem(Core *core, int64_t data, int start) {
	int mask = 0;
	int i;

	for (i=start; i<start+8; i++) {
		core->data_mem[i] = (data >> mask) &0xff;
		mask+=8;
	}
}

int64_t loadDataMem(Core *core, int start) {
	int64_t result=0;
	int i;
	int32_t mask = 0;

	for (i=start; i<start+8; i++) {
		result |= ((int64_t) core->data_mem[i]) << mask;
		mask+=8;
	}
	
	return result;
}

// Instruction Fetch
void fetch(Core *core, PipeInstr *PI) {
	PI->instruction = core->instr_mem->instructions[core->PC/4].instruction;
	core->PC += 4;
}

// Instruction Decode/Register File Read
void decode(Core* core, PipeInstr *PI) {
    // Decode instruction
	PI->dec->opcode = PI->instruction & 0x7f;
	PI->dec->rd = (PI->instruction >> 7) & 0x1f;
	PI->dec->funct3 = (PI->instruction >> 12) & 0x7;
	PI->dec->rs1 = (PI->instruction >> 15) & 0x1f;
	PI->dec->rs2 = (PI->instruction >> 20) & 0x1f;
	PI->dec->funct7 = (PI->instruction >> 25) & 0x7f;

	// Generate control signals
	ControlUnit(PI->dec->opcode, &PI->dec->ctrl_signals);

	// ALU Control unit
	PI->dec->ALU_ctrl_signal = ALUControlUnit(PI->dec->ctrl_signals.ALUOp, PI->dec->funct7, PI->dec->funct3);

	// Read values from register files
	PI->dec->reg1_val = core->reg_file[PI->dec->rs1];
	PI->dec->reg2_val = core->reg_file[PI->dec->rs2];

	// Generate immediate
	PI->dec->immediate = ImmeGen(PI->instruction);
}

// Execute stage
void execute(Core *core, PipeInstr *PI) {
	// ALU operation
	PI->ex->ALU_2nd_val = MUX(PI->dec->ctrl_signals.ALUSrc, PI->dec->reg2_val, PI->dec->immediate);
	ALU(PI->dec->reg1_val, PI->ex->ALU_2nd_val, PI->dec->ALU_ctrl_signal, &(PI->ex->ALU_result), &(PI->ex->zero), &(PI->ex->neg));
	
	/* 
	// This is for the case that there are branch instructions.
	// For a simple pipeline now, we just need to update PC by 4 at fetch every time.
    // Set PC 
	Signal branch_shift = ShiftLeft1(PI->dec->immediate);
   	Signal branch_sel=0;
	if (PI->dec->opcode == 99) {
		if (PI->dec->funct3 == 0) { // beq
			branch_sel = PI->ex->zero & PI->dec->ctrl_signals.Branch;
		} else if (PI->dec->funct3 == 1) { // bne
			branch_sel = ~PI->ex->zero & PI->dec->ctrl_signals.Branch;
		} else if (PI->dec->funct3 == 4 || PI->dec->funct3 == 6) { // blt or bltu
			branch_sel = PI->ex->neg & PI->dec->ctrl_signals.Branch;
		} else if (PI->dec->funct3 == 5 || PI->dec->funct3 == 7) { // bge or bgeu
			branch_sel = ~PI->ex->neg & PI->dec->ctrl_signals.Branch;
		}
	}
	Signal main_PC = core->PC + 4;
	Signal branch_PC = core->PC + branch_shift;
	core->PC = MUX(branch_sel, main_PC, branch_PC);
	*/
}

// Memory access stage
void memAccess(Core *core, PipeInstr *PI) {
	int64_t mem_dat = loadDataMem(core, PI->ex->ALU_result);
	PI->mem_res = MUX(PI->dec->ctrl_signals.MemtoReg, PI->ex->ALU_result, mem_dat);

	// write to memory (store)
	if (PI->dec->ctrl_signals.MemWrite) {
		storeDataMem(core, PI->mem_res, PI->dec->reg2_val);
	}
}
	
// Write back stage
void writeBack(Core *core, PipeInstr *PI) { 
	if (PI->dec->ctrl_signals.RegWrite) {
		core->reg_file[PI->dec->rd] = PI->mem_res;
	}
}

// implement this function
bool tickFunc(Core *core)
{
	int fetch_instr = 0;
	int decode_instr = -1;
	int exec_instr = -2;
	int mem_instr = -3;
	int wb_instr = -4;
	int num_instr = core->instr_mem->last->addr / 4 + 1;
	int starting_stages;
	int i;
	// create a new pipe
	core->pipe = malloc(num_instr * sizeof(PipeInstr));
	
	if (num_instr > 5) {
		starting_stages = 4;
	} else {
		starting_stages = num_instr - 1;
	}

	// Starting stages
	for (i=0; i<starting_stages; i++) {
		core->pipe[fetch_instr] = malloc(sizeof(PipeInstr));
		core->pipe[fetch_instr]->dec = malloc(sizeof(Decode));
		core->pipe[fetch_instr]->ex = malloc(sizeof(Exec));
		fetch(core, core->pipe[fetch_instr]);

		if (decode_instr >= 0)
			decode(core, core->pipe[decode_instr]);

		if (exec_instr >= 0) 
			execute(core, core->pipe[exec_instr]);

		if (mem_instr >= 0)
			memAccess(core, core->pipe[mem_instr]);

		fetch_instr++;
		decode_instr++;
		exec_instr++;
		mem_instr++;
		wb_instr++;
    	++core->clk;
	}

	// Steady stages
	while (wb_instr < num_instr) {
		if (fetch_instr < num_instr) {
			core->pipe[fetch_instr] = malloc(sizeof(PipeInstr));
			core->pipe[fetch_instr]->dec = malloc(sizeof(Decode));
			core->pipe[fetch_instr]->ex = malloc(sizeof(Exec));
			fetch(core, core->pipe[fetch_instr]);
		}

		if (decode_instr < num_instr)
			decode(core, core->pipe[decode_instr]);

		if ((exec_instr < num_instr) && (exec_instr >= 0)) 
			execute(core, core->pipe[exec_instr]);

		if ((mem_instr < num_instr) && (mem_instr >= 0)) 
			memAccess(core, core->pipe[mem_instr]);

		if (wb_instr >= 0) 
			writeBack(core, core->pipe[wb_instr]);	

		fetch_instr++;
		decode_instr++;
		exec_instr++;
		mem_instr++;
		wb_instr++;
    	++core->clk;
	}

    // Are we reaching the final instruction?
    if (core->PC > core->instr_mem->last->addr)
    {
        return false;
    }

	// free memory
	for (i=0; i<num_instr; i++) {
		free(core->pipe[i]->dec);
		free(core->pipe[i]->ex);
		free(core->pipe[i]);
	}
    return true;

	/*
	free(PI->dec);
	free(PI->ex);
	free(PI);
	*/
}

// (1). Control Unit. Refer to Figure 4.18.
void ControlUnit(Signal input,
                 ControlSignals *signals)
{
    if (input == 51) { // R-Type
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 2;
    } else if (input == 3) { // Load 
        signals->ALUSrc = 1;
        signals->MemtoReg = 1;
        signals->RegWrite = 1;
        signals->MemRead = 1;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
    } else if (input == 19) { // I-Type 
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
    } else if (input == 35) { // Store 
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 1;
        signals->Branch = 0;
        signals->ALUOp = 0;
    } else if (input == 99) { // Branch 
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 1;
        signals->ALUOp = 1;
    } else { // Default case
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
	}
}

// (2). ALU Control Unit. Refer to Figure 4.12.
Signal ALUControlUnit(Signal ALUOp,
                      Signal Funct7,
                      Signal Funct3) 
{
	if (ALUOp == 0) {
		if (Funct7 == 0) {
			if (Funct3 == 1) { // shift left
				return 4;
			} else if (Funct3 == 5) { // shift right
				return 5;
			} else if (Funct3 == 0) { // add 
				return 2; // add
			} else if (Funct3 == 7) {
				return 0; // AND 
			} else if (Funct3 == 6) {
				return 1; // OR
			} else {
				return 2;
			}
		} else if (Funct7 == 32) { // subtract
			return 6; // subtract
		} else {
			return 2;
		}
	} else if (ALUOp == 1) {
		return 6; // subtract
	} else if (ALUOp == 2) {
		if (Funct7 == 0) {
			if (Funct3 == 0) {
				return 2; // add 
			} else if (Funct3 == 7) {
				return 0; // AND 
			} else if (Funct3 == 6) {
				return 1; // OR
			} else if (Funct3 == 1) { // shift left
				return 4;
			} else if (Funct3 == 5) { // shift right
				return 5;
			}
		} else if (Funct7 == 32) {
			if (Funct3 == 0) { 
				return 6; // subtract
			}
		}
	}
	return -1;
}

// (3). Imme. Generator
Signal ImmeGen(Signal input)
{
	Signal imm_12_0;
	unsigned opcode = input & 0x7f;

	// R-type 
	if (opcode == 51) {
		return 0;
	}

	// I-type and Load type
	if (opcode == 3 || opcode == 19) {
		// Shift input 20 bits to the right to get the 12 immediate bits
		imm_12_0 = (input >> 20) & 0xfff;
	} else if (opcode == 35) { // Store type
		Signal imm_4_0 = (input >> 7) & 0x1f;
		Signal imm_11_5 = (input >> 25) & 0x7f;
		imm_12_0 = (imm_11_5 << 5) | imm_4_0;
	} else if (opcode == 99) { // Branch type
		Signal imm_12 = (input >> 31) & 0x1;
		Signal imm_10_5 = (input >> 25) & 0x3f;
		Signal imm_4_1 = (input >> 8) & 0xf;
		Signal imm_11 = (input >> 7) & 0x1;

		imm_12_0 = (imm_12 << 11) | (imm_11 << 10) | (imm_10_5 << 4) | (imm_4_1);
	}

	// If the 11th bit is 1 then add 20 1s to the right, else let it be
	if (imm_12_0 & (1 << 11)) {
		imm_12_0 = imm_12_0 | 0xfffffffffffff000;
	} 
	return imm_12_0;
}

// (4). ALU
void ALU(Signal input_0,
         Signal input_1,
         Signal ALU_ctrl_signal,
         Signal *ALU_result,
         Signal *zero,
		 Signal *neg)
{
    if (ALU_ctrl_signal == 2) { // add
        *ALU_result = (input_0 + input_1);
    } else if (ALU_ctrl_signal == 0) { // AND
		*ALU_result = (input_0 & input_1);
	} else if (ALU_ctrl_signal == 1) { // OR
		*ALU_result = input_0 | input_1;
	} else if (ALU_ctrl_signal == 6) { // subtract
		*ALU_result = input_0 - input_1;
	} else if (ALU_ctrl_signal == 4) { // shift left
		*ALU_result = input_0 << input_1;
	} else if (ALU_ctrl_signal == 5) { // shift right
		*ALU_result = input_0 >> input_1;
	}	
	
	if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
	if (*ALU_result < 0) { *neg = 1; } else { *neg = 0; }
}

// (4). MUX
Signal MUX(Signal sel,
           Signal input_0,
           Signal input_1)
{
    if (sel == 0) { return input_0; } else { return input_1; }
}

// (5). Add
Signal Add(Signal input_0,
           Signal input_1)
{
    return (input_0 + input_1);
}

// (6). ShiftLeft1
Signal ShiftLeft1(Signal input)
{
    return input << 1;
}