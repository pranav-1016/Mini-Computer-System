#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "memory.h"

#define NO_OF_REGISTERS 256
#define NO_OF_VECTOR_REGISTERS 32
#define WIDTH_OF_VECTOR_REGISTERS 8

#define CONST_VALUE_MIN 0
#define CONST_VALUE_MAX 255

extern int Register[NP][NO_OF_REGISTERS];
extern int Vector_Register[NP][NO_OF_VECTOR_REGISTERS][WIDTH_OF_VECTOR_REGISTERS];



extern int PC[NP];
extern int opcode[NP];
extern int dest[NP];
extern int src1[NP];
extern int src2[NP];

extern int end_of_simulation[NP];

extern int Z[NP];
extern int N[NP];
extern int C[NP];
extern int V[NP];

void reset(int proc_id);
void fetch(int proc_id);
void decode(int proc_id);
void execute(int proc_id);
void process_instructions(int proc_id, int count);

#endif