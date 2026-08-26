#include <stdio.h>
#include "memory.h"
#include "processor.h"

int Register[NP][NO_OF_REGISTERS] = {{0}};
int Vector_Register[NP][NO_OF_VECTOR_REGISTERS][WIDTH_OF_VECTOR_REGISTERS] = {{{0}}};

int PC[NP] = {0};
int opcode[NP] = {0};
int dest[NP] = {0};
int src1[NP] = {0};
int src2[NP] = {0};

int end_of_simulation[NP] = {0};

int Z[NP] = {0};
int N[NP] = {0};
int C[NP] = {0};
int V[NP] = {0};

static void update_flags(int proc_id, int a, int b, int result, int is_sub) {
    Z[proc_id] = (result == 0) ? 1 : 0;
    N[proc_id] = (result & (1 << 31)) ? 1 : 0;

    if (is_sub) {
        C[proc_id] = (a > b) ? 1 : 0;
        V[proc_id] = ((a >= 0 && b < 0 && result < 0) || (a < 0 && b >= 0 && result >= 0)) ? 1 : 0;
    }
    else {
        C[proc_id] = ((unsigned int)result < (unsigned int)a || (unsigned int)result < (unsigned int)b) ? 1 : 0;
        V[proc_id] = ((a >= 0 && b >= 0 && result < 0) || (a < 0 && b < 0 && result >= 0)) ? 1 : 0;
    }
}

void reset(int proc_id) {
    if (proc_id < 0 || proc_id >= NP) return;

    for (int i = 0; i < NO_OF_REGISTERS; i++) {
        Register[proc_id][i] = 0;
    }
    for (int i = 0; i < NO_OF_VECTOR_REGISTERS; i++) {
        for (int j = 0; j < WIDTH_OF_VECTOR_REGISTERS; j++) {
            Vector_Register[proc_id][i][j] = 0;
        }
    }

    PC[proc_id] = 0;
    opcode[proc_id] = 0;
    dest[proc_id] = 0;
    src1[proc_id] = 0;
    src2[proc_id] = 0;
    Z[proc_id] = 0;
    N[proc_id] = 0;
    C[proc_id] = 0;
    V[proc_id] = 0;
    end_of_simulation[proc_id] = 0;
}

void fetch(int proc_id) {
    if (proc_id < 0 || proc_id >= NP) return;

    if (PC[proc_id] < 0 || PC[proc_id] + 3 >= INSTRUCTION_MEM_SIZE) {
        printf("Core %d Fetch: PC out of bounds (PC=%d)\n", proc_id, PC[proc_id]);
        end_of_simulation[proc_id] = 1;
        return;
    }

    opcode[proc_id] = Instruction[proc_id][PC[proc_id]];
    dest[proc_id]   = Instruction[proc_id][PC[proc_id] + 1];
    src1[proc_id]   = Instruction[proc_id][PC[proc_id] + 2];
    src2[proc_id]   = Instruction[proc_id][PC[proc_id] + 3];

    printf("Core %d running instruction: %X %X %X %X\n", proc_id, opcode[proc_id], dest[proc_id], src1[proc_id], src2[proc_id]);
    PC[proc_id] += 4;
}

void decode(int proc_id) {
    (void)proc_id;
}

void execute(int proc_id) {
    if (proc_id < 0 || proc_id >= NP) return;

    int op  = opcode[proc_id];
    int d   = dest[proc_id];
    int s1  = src1[proc_id];
    int s2  = src2[proc_id];

    switch (op) {
    case OP_HALT:
        end_of_simulation[proc_id] = 1;
        break;

    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= NO_OF_REGISTERS || s2 < 0 || s2 >= NO_OF_REGISTERS) {
            printf("Core %d: Invalid register index in arithmetic (dest=%d, src1=%d, src2=%d)\n", proc_id, d, s1, s2);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (op == OP_ADD) {
            Register[proc_id][d] = Register[proc_id][s1] + Register[proc_id][s2];
            update_flags(proc_id, Register[proc_id][s1], Register[proc_id][s2], Register[proc_id][d], 0);
        }
        else if (op == OP_SUB) {
            Register[proc_id][d] = Register[proc_id][s1] - Register[proc_id][s2];
            update_flags(proc_id, Register[proc_id][s1], Register[proc_id][s2], Register[proc_id][d], 1);
        }
        else if (op == OP_MUL) {
            Register[proc_id][d] = Register[proc_id][s1] * Register[proc_id][s2];
        }
        else if (op == OP_DIV) {
            if (Register[proc_id][s2] == 0) {
                printf("Core %d Runtime error: division by zero (reg %d)\n", proc_id, s2);
                end_of_simulation[proc_id] = 1;
            }
            else {
                Register[proc_id][d] = Register[proc_id][s1] / Register[proc_id][s2];
            }
        }
        break;

    case OP_ADD_CONST:
    case OP_SUB_CONST:
    case OP_MUL_CONST:
    case OP_DIV_CONST:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= NO_OF_REGISTERS || s2 < CONST_VALUE_MIN || s2 > CONST_VALUE_MAX) {
            printf("Core %d: Invalid operand in constant arithmetic (dest=%d, src1=%d, constant=%d)\n", proc_id, d, s1, s2);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (op == OP_ADD_CONST) {
            Register[proc_id][d] = Register[proc_id][s1] + s2;
            update_flags(proc_id, Register[proc_id][s1], s2, Register[proc_id][d], 0);
        }
        else if (op == OP_SUB_CONST) {
            Register[proc_id][d] = Register[proc_id][s1] - s2;
            update_flags(proc_id, Register[proc_id][s1], s2, Register[proc_id][d], 1);
        }
        else if (op == OP_MUL_CONST) {
            Register[proc_id][d] = Register[proc_id][s1] * s2;
        }
        else if (op == OP_DIV_CONST) {
            if (s2 == 0) {
                printf("Core %d Runtime error: division by zero (constant=%d)\n", proc_id, s2);
                end_of_simulation[proc_id] = 1;
            }
            else {
                Register[proc_id][d] = Register[proc_id][s1] / s2;
            }
        }
        break;

    case OP_READ:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= NO_OF_REGISTERS) {
            printf("Core %d: Invalid registers in READ (dest=%d, addr_reg=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (Register[proc_id][s1] < 0 || Register[proc_id][s1] >= DATA_MEM_SIZE) {
            printf("Core %d: Invalid memory address in READ (address=%d)\n", proc_id, Register[proc_id][s1]);
            end_of_simulation[proc_id] = 1;
            break;
        }

        Register[proc_id][d] = Data[proc_id][Register[proc_id][s1]];
        printf("Core %d Read operation : Data[%d] = %d\n", proc_id, Register[proc_id][s1], Register[proc_id][d]);
        break;

    case OP_READ_CONST:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < CONST_VALUE_MIN || s1 > CONST_VALUE_MAX) {
            printf("Core %d: Invalid operand in READ_CONST (dest=%d, address=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (s1 >= DATA_MEM_SIZE) {
            printf("Core %d: Invalid memory address in READ_CONST (address=%d)\n", proc_id, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        Register[proc_id][d] = Data[proc_id][s1];
        printf("Core %d Read constant operation : address=%d value=%d\n", proc_id, s1, Register[proc_id][d]);
        break;

    case OP_WRITE:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= NO_OF_REGISTERS) {
            printf("Core %d: Invalid registers in WRITE (addr_reg=%d, data_reg=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (Register[proc_id][d] < 0 || Register[proc_id][d] >= DATA_MEM_SIZE) {
            printf("Core %d: Invalid memory address in WRITE (address=%d)\n", proc_id, Register[proc_id][d]);
            end_of_simulation[proc_id] = 1;
            break;
        }

        Data[proc_id][Register[proc_id][d]] = Register[proc_id][s1];
        printf("Core %d Write operation : Data[%d] = %d\n", proc_id, Register[proc_id][d], Register[proc_id][s1]);
        break;

    case OP_WRITE_CONST:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= DATA_MEM_SIZE) {
            printf("Core %d: Invalid indices in WRITE_CONST (reg=%d, addr=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        Data[proc_id][s1] = Register[proc_id][d];
        printf("Core %d Write operation : Data[%d] = %d\n", proc_id, s1, Register[proc_id][d]);
        break;

    case OP_MOV:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= NO_OF_REGISTERS) {
            printf("Core %d: Invalid register index in MOV (dest=%d, src1=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        Register[proc_id][d] = Register[proc_id][s1];
        break;

    case OP_MOV_CONST:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < CONST_VALUE_MIN || s1 > CONST_VALUE_MAX) {
            printf("Core %d: Invalid operand in MOV_CONST (dest=%d, value=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        Register[proc_id][d] = s1;
        break;

    case OP_VEC_ADD:
    case OP_VEC_SUB:
    case OP_VEC_MUL:
        if (d < 0 || d >= NO_OF_VECTOR_REGISTERS || s1 < 0 || s1 >= NO_OF_VECTOR_REGISTERS || s2 < 0 || s2 >= NO_OF_VECTOR_REGISTERS) {
            printf("Core %d: Invalid register index in vector arithmetic (dest=%d, src1=%d, src2=%d)\n", proc_id, d, s1, s2);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (op == OP_VEC_ADD) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] + Vector_Register[proc_id][s2][i];
                update_flags(proc_id, Vector_Register[proc_id][s1][i], Vector_Register[proc_id][s2][i], Vector_Register[proc_id][d][i], 0);
            }
        }
        else if (op == OP_VEC_SUB) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] - Vector_Register[proc_id][s2][i];
                update_flags(proc_id, Vector_Register[proc_id][s1][i], Vector_Register[proc_id][s2][i], Vector_Register[proc_id][d][i], 1);
            }
        }
        else if (op == OP_VEC_MUL) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] * Vector_Register[proc_id][s2][i];
                update_flags(proc_id, Vector_Register[proc_id][s1][i], Vector_Register[proc_id][s2][i], Vector_Register[proc_id][d][i], 0);
            }
        }
        break;

    case OP_VEC_ADD_CONST:
    case OP_VEC_SUB_CONST:
    case OP_VEC_MUL_CONST:
        if (d < 0 || d >= NO_OF_VECTOR_REGISTERS || s1 < 0 || s1 >= NO_OF_VECTOR_REGISTERS || s2 < CONST_VALUE_MIN || s2 > CONST_VALUE_MAX) {
            printf("Core %d: Invalid operand in constant vector arithmetic (dest=%d, src1=%d, constant=%d)\n", proc_id, d, s1, s2);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (op == OP_VEC_ADD_CONST) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] + s2;
                update_flags(proc_id, Vector_Register[proc_id][s1][i], s2, Vector_Register[proc_id][d][i], 0);
            }
        }
        else if (op == OP_VEC_SUB_CONST) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] - s2;
                update_flags(proc_id, Vector_Register[proc_id][s1][i], s2, Vector_Register[proc_id][d][i], 1);
            }
        }
        else if (op == OP_VEC_MUL_CONST) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] * s2;
                update_flags(proc_id, Vector_Register[proc_id][s1][i], s2, Vector_Register[proc_id][d][i], 0);
            }
        }
        break;

    case OP_VEC_READ:
        if (d < 0 || d >= NO_OF_VECTOR_REGISTERS || s1 < 0 || s1 >= NO_OF_REGISTERS) {
            printf("Core %d: Invalid registers in vector READ (dest=%d, addr_reg=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (Register[proc_id][s1] < 0 || Register[proc_id][s1] + (WIDTH_OF_VECTOR_REGISTERS * 4) > DATA_MEM_SIZE) {
            printf("Core %d: Invalid memory address in vector READ (address=%d)\n", proc_id, Register[proc_id][s1]);
            end_of_simulation[proc_id] = 1;
            break;
        }

        for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++, Register[proc_id][s1] += 4) {
            Vector_Register[proc_id][d][i] = Data[proc_id][Register[proc_id][s1]];
        }
        break;

    case OP_VEC_READ_CONST:
        if (d < 0 || d >= NO_OF_VECTOR_REGISTERS) {
            printf("Core %d: Invalid registers in vector READ_CONST (dest=%d)\n", proc_id, d);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (s1 < 0 || s1 + (WIDTH_OF_VECTOR_REGISTERS * 4) > DATA_MEM_SIZE) {
            printf("Core %d: Invalid memory address in vector READ_CONST (address=%d)\n", proc_id, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        {
            int addr = s1;
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++, addr += 4) {
                Vector_Register[proc_id][d][i] = Data[proc_id][addr];
            }
        }
        break;

    case OP_VEC_WRITE:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= NO_OF_VECTOR_REGISTERS) {
            printf("Core %d: Invalid registers in vector WRITE (addr_reg=%d, data_reg=%d)\n", proc_id, d, s1);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (Register[proc_id][d] < 0 || Register[proc_id][d] + (WIDTH_OF_VECTOR_REGISTERS * 4) > DATA_MEM_SIZE) {
            printf("Core %d: Invalid memory address in vector WRITE (address=%d)\n", proc_id, Register[proc_id][d]);
            end_of_simulation[proc_id] = 1;
            break;
        }

        for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++, Register[proc_id][d] += 4) {
            Data[proc_id][Register[proc_id][d]] = Vector_Register[proc_id][s1][i];
        }
        break;

    case OP_VEC_WRITE_CONST:
        if (d < 0 || d >= NO_OF_REGISTERS || s1 < 0 || s1 >= DATA_MEM_SIZE) {
            printf("Core %d: Invalid registers in vector WRITE_CONST (addr=%d, data_reg=%d)\n", proc_id, s1, d);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (Register[proc_id][d] < 0 || Register[proc_id][d] + (WIDTH_OF_VECTOR_REGISTERS * 4) > DATA_MEM_SIZE) {
            printf("Core %d: Invalid memory address in vector WRITE_CONST (address=%d)\n", proc_id, Register[proc_id][d]);
            end_of_simulation[proc_id] = 1;
            break;
        }

        {
            int addr = d;
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++, addr += 4) {
                Data[proc_id][addr] = Vector_Register[proc_id][s1][i];
            }
        }
        break;

    case OP_VEC_ADD_REG:
    case OP_VEC_SUB_REG:
    case OP_VEC_MUL_REG:
        if (d < 0 || d >= NO_OF_VECTOR_REGISTERS || s1 < 0 || s1 >= NO_OF_VECTOR_REGISTERS || s2 < 0 || s2 >= NO_OF_REGISTERS) {
            printf("Core %d: Invalid operand in vector-reg arithmetic (dest=%d, src1=%d, src2_reg=%d)\n", proc_id, d, s1, s2);
            end_of_simulation[proc_id] = 1;
            break;
        }

        if (op == OP_VEC_ADD_REG) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] + Register[proc_id][s2];
                update_flags(proc_id, Vector_Register[proc_id][s1][i], Register[proc_id][s2], Vector_Register[proc_id][d][i], 0);
            }
        }
        else if (op == OP_VEC_SUB_REG) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] - Register[proc_id][s2];
                update_flags(proc_id, Vector_Register[proc_id][s1][i], Register[proc_id][s2], Vector_Register[proc_id][d][i], 1);
            }
        }
        else if (op == OP_VEC_MUL_REG) {
            for (int i = 0; i < WIDTH_OF_VECTOR_REGISTERS; i++) {
                Vector_Register[proc_id][d][i] = Vector_Register[proc_id][s1][i] * Register[proc_id][s2];
                update_flags(proc_id, Vector_Register[proc_id][s1][i], Register[proc_id][s2], Vector_Register[proc_id][d][i], 0);
            }
        }
        break;

    case OP_BEQ:
        if (Z[proc_id] == 1) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BNE:
        if (Z[proc_id] == 0) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BCS:
        if (C[proc_id] == 1) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BCC:
        if (C[proc_id] == 0) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BMI:
        if (N[proc_id] == 1) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BPL:
        if (N[proc_id] == 0) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BVS:
        if (V[proc_id] == 1) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BVC:
        if (V[proc_id] == 0) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BHI:
        if (C[proc_id] == 1 && Z[proc_id] == 0) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BLS:
        if (C[proc_id] == 0 || Z[proc_id] == 1) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BGE:
        if (N[proc_id] == V[proc_id]) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BLT:
        if (N[proc_id] != V[proc_id]) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BGT:
        if (Z[proc_id] == 0 && N[proc_id] == V[proc_id]) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BLE:
        if (Z[proc_id] == 1 || N[proc_id] != V[proc_id]) PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;
    case OP_BAL:
        PC[proc_id] = PC[proc_id] - 4 + ((signed char)s2 * 4);
        break;

    default:
        printf("Core %d: Invalid opcode %d\n", proc_id, op);
        end_of_simulation[proc_id] = 1;
        break;
    }
}

void process_instructions(int proc_id, int count) {
    if (proc_id < 0 || proc_id >= NP) return;

    for (int i = 0; i < count; i++) {
        if (end_of_simulation[proc_id]) break;
        fetch(proc_id);
        decode(proc_id);
        execute(proc_id);
    }
}