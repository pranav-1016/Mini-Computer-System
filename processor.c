#include <stdio.h>
#include "memory.h"
#include "processor.h"

int Register[NO_OF_REGISTERS] = {0};

int PC = 0;
int opcode = 0;
int dest = 0;
int src1 = 0;
int src2 = 0;

int end_of_simulation = 0;

int Z = 0;
int N = 0;
int C = 0;
int V = 0;

void update_flags(int a, int b, int result, int is_sub) {
    Z = (result == 0) ? 1 : 0;
    N = (result & (1 << 31)) ? 1 : 0;

    if (is_sub) {
        C = (a > b) ? 1 : 0;
        V = ((a >= 0 && b < 0 && result < 0) || (a < 0 && b >= 0 && result >= 0)) ? 1 : 0;
    }
    else {
        C = ((unsigned int)result < (unsigned int)a || (unsigned int)result < (unsigned int)b) ? 1 : 0;
        V = ((a >= 0 && b >= 0 && result < 0) || (a < 0 && b < 0 && result >= 0)) ? 1 : 0;
    }
}

void reset() {
    for (int i = 0; i < NO_OF_REGISTERS; i++) {
        Register[i] = 0;
    }

    PC = 0;
    opcode = 0;
    dest = 0;
    src1 = 0;
    src2 = 0;
    Z = 0;
    N = 0;
    C = 0;
    V = 0;
    end_of_simulation = 0;
}

void fetch() {
    if (PC < 0 || PC + 3 >= INSTRUCTION_MEM_SIZE) {
        printf("Fetch: PC out of bounds (PC=%d)\n", PC);
        end_of_simulation = 1;
        return;
    }

    printf("PC Fetching at Fetch: PC = %d\n", PC);

    // printf("Instruction array: %02X %02X %02X %02X\n",
    //        Instruction[0],
    //        Instruction[1],
    //        Instruction[2],
    //        Instruction[3]);

    opcode = Instruction[PC];
    dest = Instruction[PC + 1];
    src1 = Instruction[PC + 2];
    src2 = Instruction[PC + 3];
    printf("Running this instructions : %X %X %X %X\n", opcode, dest, src1, src2);
    PC = PC + 4;
}

void decode() {}

void execute() {
    printf("EXECUTE: opcode=%02X\n", opcode);
    switch (opcode) {
    case OP_HALT:
        end_of_simulation = 1;
        break;

    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < 0 || src1 >= NO_OF_REGISTERS || src2 < 0 || src2 >= NO_OF_REGISTERS) {
            printf("Invalid register index in arithmetic (dest=%d, src1=%d, src2=%d)\n", dest, src1, src2);
            end_of_simulation = 1;
            break;
        }

        if (opcode == OP_ADD) {
            Register[dest] = Register[src1] + Register[src2];
            printf("Add operation is running %d %d %d", Register[dest], Register[src1] + Register[src2]);
            update_flags(Register[src1], Register[src2], Register[dest], 0);
        }
        else if (opcode == OP_SUB) {
            printf("SUB ENTERED: dest=%d src1=%d src2=%d\n", dest, src1, src2);
            Register[dest] = Register[src1] - Register[src2];
            printf("SUB RESULT: %d - %d = %d\n",
           Register[src1], Register[src2], Register[dest]);
            update_flags(Register[src1], Register[src2], Register[dest], 1);
            printf("SUB FLAGS: Z=%d N=%d C=%d V=%d\n", Z, N, C, V);
        }
        else if (opcode == OP_MUL) {
            Register[dest] = Register[src1] * Register[src2];
        }
        else if (opcode == OP_DIV) {
            if (Register[src2] == 0) {
                printf("Runtime error: division by zero (reg %d)\n", src2);
                end_of_simulation = 1;
            }
            else {
                Register[dest] = Register[src1] / Register[src2];
            }
        }
        break;

    case OP_ADD_CONST:
    case OP_SUB_CONST:
    case OP_MUL_CONST:
    case OP_DIV_CONST:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < 0 || src1 >= NO_OF_REGISTERS || src2 < CONST_VALUE_MIN || src2 > CONST_VALUE_MAX) {
            printf("Invalid operand in constant arithmetic (dest=%d, src1=%d, constant=%d)\n", dest, src1, src2);
            end_of_simulation = 1;
            break;
        }

        if (opcode == OP_ADD_CONST) {
            Register[dest] = Register[src1] + src2;
            update_flags(Register[src1], src2, Register[dest], 0);
        }
        else if (opcode == OP_SUB_CONST) {
            Register[dest] = Register[src1] - src2;
            update_flags(Register[src1], src2, Register[dest], 1);
        }
        else if (opcode == OP_MUL_CONST) {
            Register[dest] = Register[src1] * src2;
        }
        else if (opcode == OP_DIV_CONST) {
            if (src2 == 0) {
                printf("Runtime error: division by zero (constant=%d)\n", src2);
                end_of_simulation = 1;
            }
            else {
                Register[dest] = Register[src1] / src2;
            }
        }
        break;

    case OP_READ:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < 0 || src1 >= NO_OF_REGISTERS) {
            printf("Invalid registers in READ (dest=%d, addr_reg=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }

        if (Register[src1] < 0 || Register[src1] >= DATA_MEM_SIZE) {
            printf("Invalid memory address in READ (address=%d)\n", Register[src1]);
            end_of_simulation = 1;
            break;
        }

        Register[dest] = Data[Register[src1]];

        printf("Read operation : Data[%d] = %d\n", Register[src1], Register[dest]);
    break;

    case OP_READ_CONST:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < CONST_VALUE_MIN || src1 > CONST_VALUE_MAX) {
            printf("Invalid operand in READ_CONST (dest=%d, address=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }

        if (src1 >= DATA_MEM_SIZE) {
            printf("Invalid memory address in READ_CONST (address=%d)\n", src1);
            end_of_simulation = 1;
            break;
        }

        Register[dest] = Data[src1];
        printf("Read constant operation : address=%d value=%d\n", src1, Register[dest]);
        break;

    case OP_WRITE:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < 0 || src1 >= NO_OF_REGISTERS) {
            printf("Invalid registers in WRITE (addr_reg=%d, data_reg=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }

        if (Register[dest] < 0 || Register[dest] >= DATA_MEM_SIZE) {
            printf("Invalid memory address in WRITE (address=%d)\n", Register[dest]);
            end_of_simulation = 1;
            break;
        }

        Data[Register[dest]] = Register[src1];

        printf("Write operation : Data[%d] = %d\n", Register[dest], Register[src1]);
        break;

    case OP_WRITE_CONST:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < 0 || src1 >= DATA_MEM_SIZE) {
            printf("Invalid indices in WRITE_CONST (reg=%d, addr=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }

        Data[src1] = Register[dest];

        printf("Write operation : Data[%d] = %d\n", src1, Register[dest]);
        break;

    case OP_MOV:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < 0 || src1 >= NO_OF_REGISTERS) {
            printf("Invalid register index in MOV (dest=%d, src1=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }

        Register[dest] = Register[src1];
        break;

    case OP_MOV_CONST:
        if (dest < 0 || dest >= NO_OF_REGISTERS || src1 < CONST_VALUE_MIN || src1 > CONST_VALUE_MAX) {
            printf("Invalid operand in MOV_CONST (dest=%d, value=%d)\n", dest, src1);
            end_of_simulation = 1;
            break;
        }

        Register[dest] = src1;
        break;

    case OP_BEQ:
        if (Z == 1) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BNE:
        if (Z == 0) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BCS:
        if (C == 1) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BCC:
        if (C == 0) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BMI:
        if (N == 1) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BPL:
        if (N == 0) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BVS:
        if (V == 1) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BVC:
        if (V == 0) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BHI:
        if (C == 1 && Z == 0) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BLS:
        if (C == 0 || Z == 1) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BGE:
        if (N == V) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BLT:
        if (N != V) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BGT:
        if (Z == 0 && N == V) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BLE:
        if (Z == 1 || N != V) {
            PC = PC - 4 + ((signed char)src2 * 4);
        }
        break;

    case OP_BAL:
        printf("PC on BAL : %d\n", PC);

        PC = PC - 4 + ((signed char)src2 * 4);

        printf("new PC on BAL : %d\n", PC);
        break;
    default:
        printf("Invalid opcode: %d\n", opcode);
        end_of_simulation = 1;
        break;
    }
}