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

	/* ../cpu_traces/project_four  *
	storeDataMem(core, -63, 40);
	storeDataMem(core, 63, 48);

	core->reg_file[1] = 0;
	core->reg_file[2] = 10;
	core->reg_file[3] = -15;
	core->reg_file[4] = 20;
	core->reg_file[5] = 30;
	core->reg_file[6] = -35;
	********************************/

	/* ../cpu_traces/project_five  */
	storeDataMem(core, 100, 40);

	core->reg_file[1] = 0;
	core->reg_file[5] = 26;
	core->reg_file[6] = -27;
	/*******************************/

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
void execute(Core *core, PipeInstr *PI, int pipe_idx) {
	// Read values from register files
	PI->dec->reg1_val = core->reg_file[PI->dec->rs1];
	PI->dec->reg2_val = core->reg_file[PI->dec->rs2];

	// Forward signals
	Signal forwardA = 0;
	Signal forwardB = 0;
	Signal val1 = PI->dec->reg1_val;
	Signal val2 = PI->dec->reg2_val;
	
	if (PI->dec->rs1 != 0) {
		if (pipe_idx >= 1) { 
			if (core->pipe[pipe_idx-1]->dec->ctrl_signals.RegWrite 
				&& core->pipe[pipe_idx-1]->dec->rd == PI->dec->rs1) {	
				// Forward from execute stage
				forwardA = 2;
				if (core->pipe[pipe_idx-1]->dec->opcode == 3) { // previous instr was a load type
					val1 = core->pipe[pipe_idx-1]->mem_res;
				} else { 
					val1 = core->pipe[pipe_idx-1]->ex->ALU_result;	
				}
			}
		} 
		if (pipe_idx >= 2) {	
			if (core->pipe[pipe_idx-2]->dec->ctrl_signals.RegWrite 
					&& core->pipe[pipe_idx-2]->dec->rd == PI->dec->rs1) {	
				forwardA = 1;
				// Forward from memory stage
				val1 = core->pipe[pipe_idx-2]->mem_res;
			}
		}
	}

	if (PI->dec->rs2 != 0) {
		if (pipe_idx >= 1) { 
			if (core->pipe[pipe_idx-1]->dec->ctrl_signals.RegWrite 
				&& core->pipe[pipe_idx-1]->dec->rd == PI->dec->rs2) {	
				// Forward from execute stage
				forwardB = 2;
				if (core->pipe[pipe_idx-1]->dec->opcode == 3) { // previous instr was a load type
					val2 = core->pipe[pipe_idx-1]->mem_res;
				} else { 
					val2 = core->pipe[pipe_idx-1]->ex->ALU_result;	
				}
			}
		} 
		if (pipe_idx >= 2) {	
			if (core->pipe[pipe_idx-2]->dec->ctrl_signals.RegWrite 
					&& core->pipe[pipe_idx-2]->dec->rd == PI->dec->rs2) {	
				// Forward from memory stage
				forwardB = 1;
				val2 = core->pipe[pipe_idx-2]->mem_res;
			}
		}
	}

	// ALU operation
	PI->ex->ALU_2nd_val = MUX(PI->dec->ctrl_signals.ALUSrc, val2, PI->dec->immediate);
	ALU(val1, PI->ex->ALU_2nd_val, PI->dec->ALU_ctrl_signal, &(PI->ex->ALU_result), &(PI->ex->zero), &(PI->ex->neg));

	if (forwardA) {
		printf("In execute stage of instruction [%d], forwardA = %ld.\n", pipe_idx + 1, forwardA);
	} 
	if (forwardB) {
		printf("In execute stage of instruction [%d], forwardB = %ld.\n", pipe_idx + 1, forwardB);
	}
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
	int stall_ex = 0;
	int stall_mem = 0;
	int stall_wb = 0;
	int i;
	// create a new pipe
	core->pipe = malloc(num_instr * sizeof(PipeInstr));
	
	// pipeline
	while (wb_instr < num_instr) {
		printf("======================== Clock cycle %ld ========================\n", core->clk+1);

		if (fetch_instr < num_instr) {
			core->pipe[fetch_instr] = malloc(sizeof(PipeInstr));
			core->pipe[fetch_instr]->dec = malloc(sizeof(Decode));
			core->pipe[fetch_instr]->ex = malloc(sizeof(Exec));
			fetch(core, core->pipe[fetch_instr]);
			printf("Fetched instruction [%d].\n", fetch_instr + 1);
				printf("-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-\n");
		}

		if (decode_instr >= 0 && decode_instr < num_instr) {
			decode(core, core->pipe[decode_instr]);
			if (decode_instr >= 1) {
				if ((core->pipe[decode_instr-1]->dec->opcode == 3)
					&& (core->pipe[decode_instr-1]->dec->rd == core->pipe[decode_instr]->dec->rs1 || core->pipe[decode_instr-1]->dec->rd == core->pipe[decode_instr]->dec->rs2)) {
					// insert a bubble if the previous instruction is a load type and introduces data hazards
					stall_ex = 1;
					printf("Inserting a bubble in the next clock cycle because of data hazard.\n");
				}
			}
			printf("Decoded instruction [%d].\n", decode_instr + 1);
				printf("-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-\n");
		}	

		if ((exec_instr < num_instr) && (exec_instr >= 0)) {
			if (!stall_mem) { 
				execute(core, core->pipe[exec_instr], exec_instr);
				printf("Executed instruction [%d].\n", exec_instr + 1);
				printf("-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-\n");
			} 
		}

		if ((mem_instr < num_instr) && (mem_instr >= 0)) {
			if (!stall_wb) {
				memAccess(core, core->pipe[mem_instr]);
				printf("Accessed memory for instruction [%d].\n", mem_instr + 1);
				printf("-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-\n");
			}
		}

		if (wb_instr >= 0) {  
			writeBack(core, core->pipe[wb_instr]);	
			printf("Wrote back to register for instruction [%d].\n", wb_instr + 1);
			if (core->pipe[wb_instr]->dec->ctrl_signals.RegWrite) {
				printf("New register value: x[%ld] = %ld.\n", core->pipe[wb_instr]->dec->rd, core->pipe[wb_instr]->mem_res);
				printf("-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-\n");
			}
		}

		printf("\n");
		// ensuring that the bubble is inserted for every instruction afterwards
		if (stall_ex) {
			exec_instr--;
			stall_ex = 0;
			stall_mem = 1;
		} else if (stall_mem) {
			mem_instr--;
			stall_mem = 0;
			stall_wb = 1;
		} else if (stall_wb) {
			wb_instr--;
			stall_wb = 0;
		}

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
// Mux 3
Signal MUX3(Signal sel,
			Signal input_0,
			Signal input_1,
			Signal input_2) {
	if (sel == 0) {
		return input_0;
	} else if (sel == 1) {
		return input_1;
	} else {
		return input_2;
	}
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
