#include <stdio.h>
#include "memory.h"

int Register[MEM_SIZE];
int PC, opcode, dest, src1, src2;
int end_of_simulation = 0;

void reset() {
    for (int i = 0; i < MEM_SIZE; i++) {
        Register[i] = 0;
    }
    PC = 0;
    opcode = 0;
    dest = 0;
    src1 = 0;
    src2 = 0;
}

void fetch() {
    if (PC < 0 || PC + 3 >= MEM_SIZE) {
        printf("Fetch: PC out of bounds (PC=%d)\n", PC);
        end_of_simulation = 1;
        return;
    }
    opcode = Instruction[PC];
    dest = Instruction[PC + 1];
    src1 = Instruction[PC + 2];
    src2 = Instruction[PC + 3];

    PC = PC + 4;
}

void decode() {}

void execute() {
    switch (opcode) {
    case OP_HALT:
        end_of_simulation = 1;
        break;

    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        if (dest < 0 || dest >= MEM_SIZE || src1 < 0 || src1 >= MEM_SIZE || src2 < 0 || src2 >= MEM_SIZE) {
            printf("Invalid register index in arithmetic (dest=%d, src1=%d, src2=%d)\n", dest, src1, src2);
            end_of_simulation = 1;
            break;
        }
        if (opcode == OP_ADD)
            Register[dest] = Register[src1] + Register[src2];
        else if (opcode == OP_SUB)
            Register[dest] = Register[src1] - Register[src2];
        else if (opcode == OP_MUL)
            Register[dest] = Register[src1] * Register[src2];
        else if (opcode == OP_DIV) {
            if (Register[src2] == 0) {
                printf("Runtime error: division by zero (reg %d)\n", src2);
                end_of_simulation = 1;
            } else {
                Register[dest] = Register[src1] / Register[src2];
            }
        }
        break;

    case OP_READ:
        if (dest < 0 || dest >= MEM_SIZE || src1 < 0 || src1 >= MEM_SIZE) {
            printf("Invalid indices in READ (reg=%d, addr=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }
        Register[dest] = Data[src1];
        printf("Read operation : %d %d\n", src1, Data[src1]);
        break;

    case OP_WRITE:
        if (dest < 0 || dest >= MEM_SIZE || src1 < 0 || src1 >= MEM_SIZE) {
            printf("Invalid indices in WRITE (reg=%d, addr=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }
        Data[src1] = Register[dest];
        printf("Write operation : %d %d\n", src1, Register[dest]);
        break;

    case OP_MOV:
        if (dest < 0 || dest >= MEM_SIZE) {
            printf("Invalid dest in MOV (dest=%d)\n", dest);
            end_of_simulation = 1;
            break;
        }
        Register[dest] = src1;
        break;

    default:
        printf("Invalid opcode: %d\n", opcode);
        end_of_simulation = 1;
        break;
    }
}
