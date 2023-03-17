#include <stdio.h>

#include "Core.h"
#include "Parser.h"

// Function to print out bytes in binary form
void print_byte(Byte n) {
	int i;
	for (i=7; i>=0; i--) {
		printf("%d", (n >> i) & 1);
	}
}

int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<trace-file>");

        return 0;
    }

	int i;

    /* Task One */
    // (1) parse and translate all the assembly instructions into binary format;
    // (2) store the translated binary instructions into instruction memory.
    Instruction_Memory instr_mem;
    instr_mem.last = NULL;
    loadInstructions(&instr_mem, argv[1]);
    unsigned PC = 0;
    while (1)
    {
        Instruction *instr = &(instr_mem.instructions[PC / 4]);
        printf("\nInstruction at PC: %u\n", PC);
        unsigned mask = (1 << 31);
        for (int i = 31; i >= 0; i--)
        {
            if (instr->instruction & mask) { printf("1");}
            else { printf("0"); }

            mask >>= 1;
        }
        printf("\n");
        if (instr == instr_mem.last) { break; }
        PC += 4;
    }

	printf("\n*----------------------------------------------*\n");
    /* Task Two */
    // implement Core.{h,c}
    Core *core = initCore(&instr_mem);

	// Print original values
	printf("\nOriginal register values (only values != 0):\n");
	for (i=0; i<32; i++) {
		if (core->reg_file[i]) {
			printf("x[%d]: %ld\n", i, core->reg_file[i]);
		}
	}

	printf("\nOriginal memory bytes (only values != 0):\n");
	for (i=0; i<1024; i++) {
		if (core->data_mem[i]) {
			printf("Mem[%d]: ", i);
			print_byte(core->data_mem[i]);
			printf("\n");
		}
	}

	printf("\n*----------------------------------------------*\n");
    /* Task Three - Simulation */
    while (core->tick(core)) {
	}

	printf("\nNumber of clock cycles: %ld\n", core->clk);
	printf("\n");
	printf("*----------------------------------------------*\n");


	// Print final values
	printf("\nFinal register values (only values != 0):\n");
	for (i=0; i<32; i++) {
		if (core->reg_file[i]) {
			printf("x[%d]: %ld\n", i, core->reg_file[i]);
		}
	}

	printf("\nFinal memory bytes (only values != 0):\n");
	for (i=0; i<1024; i++) {
		if (core->data_mem[i]) {
			printf("Mem[%d]: ", i);
			print_byte(core->data_mem[i]);
			printf("\n");
		}
	}


	printf("\n");
    printf("Simulation is finished.\n");

    free(core);    
}
