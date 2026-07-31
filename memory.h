#ifndef MEMORY_H
#define MEMORY_H

/* Memory and VM constants */
#define MEM_SIZE 256

/* Opcode constants */
#define OP_HALT 0
#define OP_ADD 1
#define OP_SUB 2
#define OP_MUL 3
#define OP_DIV 4
#define OP_READ 5
#define OP_WRITE 6
#define OP_MOV 7

extern unsigned char Instruction[MEM_SIZE];
extern int Data[MEM_SIZE];

void initialise();
void finalize();

#endif