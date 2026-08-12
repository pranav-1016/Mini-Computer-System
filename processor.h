#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "memory.h"

#define NO_OF_REGISTERS 256

#define CONST_VALUE_MIN 0
#define CONST_VALUE_MAX 255

extern int Register[NO_OF_REGISTERS];

extern int PC;
extern int opcode;
extern int dest;
extern int src1;
extern int src2;

extern int end_of_simulation;

extern int Z;
extern int N;
extern int C;
extern int V;

void reset();
void fetch();
void decode();
void execute();

#endif