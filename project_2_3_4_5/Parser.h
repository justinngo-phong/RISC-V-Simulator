#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Instruction_Memory.h"
#include "Registers.h"

void loadInstructions(Instruction_Memory *i_mem, const char *trace);
void parseRType(char *opr, Instruction *instr);
void parseIType(char *opr, Instruction *instr);
void parseLoadType(char *opr, Instruction *instr);
void parseBType(char *opr, Instruction *instr);
int kBitsFrom(int number, int k, int p);
int kthBit(int number, int k);
int regIndex(char *reg);
