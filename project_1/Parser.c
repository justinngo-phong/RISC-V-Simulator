#include "Parser.h"

/*------------------ Parser.c ------------------
 |  
 |  Author: Justin  Ngo
 |  Written on: 1/28/2023 
 |  
 |  Purpose: Parse R-, I-, Load-, B- Type RISC-V
 |		instructions into their binary representation
 |		according to the RISC-V data ref card
 |
 *----------------------------------------------*/

// Function to load instructions and decide which type to parse
void loadInstructions(Instruction_Memory *i_mem, const char *trace)
{
    printf("Loading trace file: %s\n", trace);
	
    FILE *fd = fopen(trace, "r");
    if (fd == NULL)
    {
        perror("Cannot open trace file. \n");
        exit(EXIT_FAILURE);
    }

    // Iterate all the assembly instructions
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
	char *raw_instr;

    Addr PC = 0; // program counter points to the zeroth location initially.
    int IMEM_index = 0;
    while ((read = getline(&line, &len, fd)) != -1)
    {
		printf("%s\n", line);
        // Assign program counter
        i_mem->instructions[IMEM_index].addr = PC;

        // Extract operation
        raw_instr = strtok(line, " ");

        if (strcmp(raw_instr, "add") == 0 ||
            strcmp(raw_instr, "sub") == 0 ||
            strcmp(raw_instr, "sll") == 0 ||
            strcmp(raw_instr, "srl") == 0 ||
            strcmp(raw_instr, "xor") == 0 ||
            strcmp(raw_instr, "or")  == 0 ||
            strcmp(raw_instr, "and") == 0) {
			// R-Type instructions
            parseRType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
		} else if (strcmp(raw_instr, "addi") == 0 ||
				   strcmp(raw_instr, "slli") == 0 ||
				   strcmp(raw_instr, "slti") == 0 ||
				   strcmp(raw_instr, "sltiu")== 0 ||
				   strcmp(raw_instr, "xori") == 0 ||
				   strcmp(raw_instr, "srli") == 0 ||
				   strcmp(raw_instr, "srai") == 0 ||
				   strcmp(raw_instr, "ori")  == 0 ||
				   strcmp(raw_instr, "andi") == 0 ||
				   strcmp(raw_instr, "addiw")== 0 ||
				   strcmp(raw_instr, "slliw")== 0 ||
				   strcmp(raw_instr, "srliw")== 0 ||
				   strcmp(raw_instr, "sraiw")== 0) {
			// I-Type instructions
            parseIType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
		} else if (strcmp(raw_instr, "ld") == 0 ||
				   strcmp(raw_instr, "lb") == 0 ||	
				   strcmp(raw_instr, "lh") == 0 ||	
				   strcmp(raw_instr, "lw") == 0 ||	
				   strcmp(raw_instr, "lbu")== 0 ||	
				   strcmp(raw_instr, "lhu")== 0 ||	
				   strcmp(raw_instr, "lwu")== 0){
			// Load Type instructions
            parseLoadType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
		} else if (strcmp(raw_instr, "bne") == 0 ||
				   strcmp(raw_instr, "beq") == 0 ||
				   strcmp(raw_instr, "blt") == 0 ||
				   strcmp(raw_instr, "bge") == 0 ||
				   strcmp(raw_instr, "bltu")== 0 ||
				   strcmp(raw_instr, "bgeu")== 0){
			// B-Type instructions
            parseBType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
		}
		
        IMEM_index++;
        PC += 4;
    }

    fclose(fd);
}

// Function to parse R-Type instructions
void parseRType(char *opr, Instruction *instr) {
    instr->instruction = 0;
    unsigned opcode = 0;
    unsigned funct3 = 0;
    unsigned funct7 = 0;

    if (strcmp(opr, "add") == 0) {
        opcode = 51;
        funct3 = 0;
        funct7 = 0;
    } else if (strcmp(opr, "sub") == 0) {
		opcode = 51;
		funct3 = 0;
		funct7 = 32;
    } else if (strcmp(opr, "sll") == 0) {
		opcode = 51;
		funct3 = 1;
		funct7 = 0;
    } else if (strcmp(opr, "srl") == 0) {
		opcode = 51;
		funct3 = 5;
		funct7 = 0;
    } else if (strcmp(opr, "xor") == 0) {
		opcode = 51;
		funct3 = 4;
		funct7 = 0;
    } else if (strcmp(opr, "or") == 0) {
		opcode = 51;
		funct3 = 6;
		funct7 = 0;
    } else if (strcmp(opr, "and") == 0) {
		opcode = 51;
		funct3 = 7;
		funct7 = 0;
	}

    char *reg = strtok(NULL, ", ");
    unsigned rd = regIndex(reg);

    reg = strtok(NULL, ", ");
    unsigned rs_1 = regIndex(reg);

    reg = strtok(NULL, ", ");
    reg[strlen(reg)-1] = '\0';
    unsigned rs_2 = regIndex(reg);

    // Contruct instruction
    instr->instruction |= opcode;
    instr->instruction |= (rd << 7);
    instr->instruction |= (funct3 << (7 + 5));
    instr->instruction |= (rs_1 << (7 + 5 + 3));
    instr->instruction |= (rs_2 << (7 + 5 + 3 + 5));
    instr->instruction |= (funct7 << (7 + 5 + 3 + 5 + 5));
}

// Function to parse I-Type instructions
void parseIType(char *opr, Instruction *instr) {
	instr->instruction = 0;
	unsigned opcode = 0;
	unsigned funct3 = 0;
	signed imm = 0;

	if (strcmp(opr, "addi") == 0) {
		// Example: addi x22, x22, 1
		opcode = 19;
		funct3 = 0;
	} else if (strcmp(opr, "slli") == 0) {
		// Example: slli x11, x22, 3
		opcode = 19;
		funct3 = 1;
	} else if (strcmp(opr, "slti") == 0) {
		opcode = 19;
		funct3 = 2;
	} else if (strcmp(opr, "sltiu") == 0) {
		opcode = 19;
		funct3 = 3;
	} else if (strcmp(opr, "xori") == 0) {
		opcode = 19;
		funct3 = 4;
	} else if (strcmp(opr, "srli") == 0) {
		opcode = 19;
		funct3 = 5;
		imm = 0;
	} else if (strcmp(opr, "srai") == 0) {
		opcode = 19;
		funct3 = 5;
		imm = 32 << 5;
	} else if (strcmp(opr, "ori") == 0) {
		opcode = 19;
		funct3 = 6;
	} else if (strcmp(opr, "andi") == 0) {
		opcode = 19;
		funct3 = 7;
	} else if (strcmp(opr, "addiw") == 0) {
		opcode = 23;
		funct3 = 0;
	} else if (strcmp(opr, "slliw") == 0) {
		opcode = 23;
		funct3 = 1;
		imm = 0;
	} else if (strcmp(opr, "srliw") == 0) {
		opcode = 23;
		funct3 = 5;
		imm = 0;
	} else if (strcmp(opr, "sraiw") == 0) {
		opcode = 23;
		funct3 = 5;
		imm = 32 << 5;
	}
	
	
	// Take the destination register address (rd)
	char* reg = strtok(NULL, ", ");
	unsigned rd = regIndex(reg);

	// Take the base register address (rs_1)
	reg = strtok(NULL, ", ");
	unsigned rs_1 = regIndex(reg);

	// Take the immidiate value (imm)
	reg = strtok(NULL, ", ");
	imm |= atoi(reg);

	// Construct instruction binary rep
	instr->instruction |= opcode;
	instr->instruction |= (rd << 7);
	instr->instruction |= (funct3 << (7+5));
	instr->instruction |= (rs_1 << (7+5+3));
	instr->instruction |= (imm << (7+5+3+5));
}

// Function to parse Load Type instructions
void parseLoadType(char *opr, Instruction *instr) {
	instr->instruction = 0;
	unsigned opcode = 0;
	unsigned funct3 = 0;

	if (strcmp(opr, "lb") == 0) {
		opcode = 3;
		funct3 = 0;
	} else if (strcmp(opr, "lh") == 0) {
		opcode = 3;
		funct3 = 1;
	} else if (strcmp(opr, "lw") == 0) {
		opcode = 3;
		funct3 = 2;
	} else if (strcmp(opr, "ld") == 0) {
		// Example: ld x9, 0(x10) 
		opcode = 3;
		funct3 = 3;
	} else if (strcmp(opr, "lbu") == 0) {
		opcode = 3;
		funct3 = 4;
	} else if (strcmp(opr, "lhu") == 0) {
		opcode = 3;
		funct3 = 5;
	} else if (strcmp(opr, "lwu") == 0) {
		opcode = 3;
		funct3 = 6;
	}

	// Take the destination register address (rd)
	char* reg = strtok(NULL, ", ");
	unsigned rd = regIndex(reg);
	
	// Take the immidiate value (imm)
	reg = strtok(NULL, "(");
	signed imm = atoi(reg); 

	// Take the base register address (rs_1)
	reg = strtok(NULL, ")");
	unsigned rs_1 = regIndex(reg);

	// Construct instruction binary rep
	instr->instruction |= opcode;
	instr->instruction |= (rd << 7);
	instr->instruction |= (funct3 << (7+5));
	instr->instruction |= (rs_1 << (7+5+3));
	instr->instruction |= (imm << (7+5+3+5));
}

// Function to parse B Type instructions
void parseBType(char *opr, Instruction *instr) {
	instr->instruction = 0;
	unsigned opcode = 0;
	unsigned funct3 = 0;
	if (strcmp(opr, "beq") == 0) {
		opcode = 99;
		funct3 = 0;
	} else if (strcmp(opr, "bne") == 0) {
		// Example: bne x8, x24, -4
		opcode = 99;
		funct3 = 1;
	} else if (strcmp(opr, "blt") == 0) {
		opcode = 99;
		funct3 = 4;
	} else if (strcmp(opr, "bge") == 0) {
		opcode =  99;
		funct3 = 5;
	} else if (strcmp(opr, "bltu") == 0) {
		opcode =  99;
		funct3 = 6;
	} else if (strcmp(opr, "bgeu") == 0) {
		opcode = 99;
		funct3 = 7;
	}

	// Take reg addr 1 (rs_1)
	char* reg = strtok(NULL, ", ");
	unsigned rs_1 = regIndex(reg);

	// Take reg addr 2 (rs_2)
	reg = strtok(NULL, ", ");
	unsigned rs_2 = regIndex(reg);

	// Take immediate value
	reg = strtok(NULL, ", ");
	signed imm = atoi(reg);

	// Construct instruction binary rep
	instr->instruction |= opcode;
	instr->instruction |= (kthBit(imm, 11) << 7);
	instr->instruction |= (kBitsFrom(imm, 4, 1) << (7+1));
	instr->instruction |= (funct3 << (7+1+4));
	instr->instruction |= (rs_1 << (7+1+4+3));
	instr->instruction |= (rs_2 << (7+1+4+3+5));
	instr->instruction |= (kBitsFrom(imm, 6, 5) << (7+1+4+3+5+5));
	instr->instruction |= (kthBit(imm,12) << (7+1+4+3+5+5+6));
	
}

// Let the rightmost bit be bit 0
// Function to extract k bits from p position
// and returns the extracted value as integer
int kBitsFrom(int number, int k, int p) {
	return (((1 << k) - 1) & (number >> p));
}

// Let the rightmost bit be bit 0
// Function to extract k-th bit from integer
int kthBit(int number, int k) {
	return (number >> k) & 1;
}

// Function to extract the index of the reg address
int regIndex(char *reg)
{
    unsigned i = 0;
    for (i; i < NUM_OF_REGS; i++)
    {
        if (strcmp(REGISTER_NAME[i], reg) == 0)
        {
            break;
        }
    }

    return i;
}
