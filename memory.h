#ifndef MEMORY_H
#define MEMORY_H

/* Memory constants */
#define INSTRUCTION_MEM_SIZE 256
#define DATA_MEM_SIZE 4096
#define MEMSIZE 8192
#define PAGESIZE 512

#define NP 4

/* Opcode constants */
#define OP_HALT 0x00

// Operations with variable operands
#define OP_ADD 0x01
#define OP_SUB 0x02
#define OP_MUL 0x03
#define OP_DIV 0x04

#define OP_READ 0x05
#define OP_WRITE 0x06
#define OP_MOV 0x07
#define OP_PRINT 0x08

// Operations with constant as second operand
#define OP_ADD_CONST 0x09
#define OP_SUB_CONST 0x0A
#define OP_MUL_CONST 0x0B
#define OP_DIV_CONST 0x0C
#define OP_READ_CONST 0x0D
#define OP_WRITE_CONST 0x0E
#define OP_MOV_CONST 0x0F

// Vector Instructions
#define OP_VEC_ADD 0x21
#define OP_VEC_SUB 0x22
#define OP_VEC_MUL 0x23
#define OP_VEC_READ 0x24
#define OP_VEC_WRITE 0x25

#define OP_VEC_ADD_CONST 0x29
#define OP_VEC_SUB_CONST 0x2A
#define OP_VEC_MUL_CONST 0x2B
#define OP_VEC_READ_CONST 0x2C
#define OP_VEC_WRITE_CONST 0x2E

// Vector instructions where register value is added to vector
#define OP_VEC_ADD_REG 0x3A
#define OP_VEC_SUB_REG 0x3B
#define OP_VEC_MUL_REG 0x3C

// Branch instructions
#define OP_BEQ 0x10
#define OP_BNE 0x11
#define OP_BCS 0x12
#define OP_BCC 0x13
#define OP_BMI 0x14
#define OP_BPL 0x15
#define OP_BVS 0x16
#define OP_BVC 0x17
#define OP_BHI 0x18
#define OP_BLS 0x19
#define OP_BGE 0x1A
#define OP_BLT 0x1B
#define OP_BGT 0x1C
#define OP_BLE 0x1D
#define OP_BAL 0x1E

// extern unsigned char Instruction[NP][INSTRUCTION_MEM_SIZE];
// extern int Data[NP][DATA_MEM_SIZE];
extern char memory[MEMSIZE];

void load_the_program(int proc_id, const char *filename);
void load_the_data(int proc_id, const char *filename);
void initialise(int proc_id, const char *program_file, const char *data_file);
void finalize(int proc_id, const char *filename);

#endif