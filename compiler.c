#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "compiler.h"
#include "memory.h"

#define MAX_LINES 100
#define MAX_LABELS 64
#define MAX_LABEL_LENGTH 50

typedef struct {
    char name[MAX_LABEL_LENGTH];
    int instruction_index;
} Label;

Label labels[MAX_LABELS];
int label_count = 0;

int find_label(const char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) {
            return labels[i].instruction_index;
        }
    }
    return -1;
}

char* extract_filename(const char *filename) {
    if (filename == NULL) return NULL;

    printf("Entering extract filename function: %s\n", filename);

    // Create a writable heap copy of the read-only string
    char *filename_copy = strdup(filename);
    if (filename_copy == NULL) return NULL;

    char *token = strtok(filename_copy, ".");
    
    // Allocate buffer for returned string so we can free the original copy
    char *result = NULL;
    if (token != NULL) {
        result = strdup(token);
    }

    free(filename_copy); // Clean up the working copy

    printf("Extracted token: %s\n", result);
    return result; // Remember to free(result) in the caller!
}

int get_branch_opcode(const char *branch) {
    if (strcmp(branch, "BEQ") == 0) return 0x10;
    if (strcmp(branch, "BNE") == 0) return 0x11;
    if (strcmp(branch, "BCS") == 0) return 0x12;
    if (strcmp(branch, "BCC") == 0) return 0x13;
    if (strcmp(branch, "BMI") == 0) return 0x14;
    if (strcmp(branch, "BPL") == 0) return 0x15;
    if (strcmp(branch, "BVS") == 0) return 0x16;
    if (strcmp(branch, "BVC") == 0) return 0x17;
    if (strcmp(branch, "BHI") == 0) return 0x18;
    if (strcmp(branch, "BLS") == 0) return 0x19;
    if (strcmp(branch, "BGE") == 0) return 0x1A;
    if (strcmp(branch, "BLT") == 0) return 0x1B;
    if (strcmp(branch, "BGT") == 0) return 0x1C;
    if (strcmp(branch, "BLE") == 0) return 0x1D;
    if (strcmp(branch, "BAL") == 0) return 0x1E;

    return -1;
}

int is_valid_register(int reg) {
    return reg >= 0 && reg <= 255;
}
int is_valid_vector_register(int reg) {
    return reg >= 0 && reg <= 31;
}

int is_valid_constant(int value) {
    return value >= 0 && value <= 255;
}

int is_valid_label(const char *label) {
    if (label[0] != '.') {
        return 0;
    }

    if (label[1] == '\0') {
        return 0;
    }

    for (int i = 1; label[i] != '\0'; i++) {
        if (!isalnum((unsigned char)label[i])) {
            return 0;
        }
    }

    return 1;
}
int isBlankLine(const char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] != ' ' &&
            line[i] != '\t' &&
            line[i] != '\n' &&
            line[i] != '\r')
        {
            return 0;   // not blank
        }
    }

    return 1;           // completely blank
}

void print_labels() {
    printf("Labels found (%d):\n", label_count);
    if (label_count == 0) {
        printf("  <none>\n");
        return;
    }

    for (int i = 0; i < label_count; i++) {
        printf("  %s -> instruction %d\n", labels[i].name, labels[i].instruction_index);
    }
}

char* compile(const char *filename) {
    // extract the filename without extension
    printf("Enterting the compile method with filename %s  ....\n", filename);
    char *raw_filename = extract_filename(filename);
    printf("this will be the filename for program byte file %s", raw_filename);
    FILE *input = fopen(filename, "r");
    FILE *output = fopen(raw_filename, "w");

    printf("%s filename will be opened\n", filename);
    if (input == NULL) {
        printf("File not found or not opened >>>>>\n");
        return;
    }
    printf("%s output file will be created\n", raw_filename);
    if (output == NULL) {
        printf("Unable to create program.byte >>>>>\n");
        fclose(input);
        return;
    }

    printf("Compiling %s...\n", filename);

    char buffer[100];
    int instruction_index = 0;

    /*
     * PASS 1:
     * Find all labels and determine their instruction positions.
     */
    printf("Pass 1: scanning labels and instruction positions...\n");
    label_count = 0;
    while (fgets(buffer, sizeof(buffer), input)) {
        char *comment = strchr(buffer, '%');

        if (comment != NULL) {
            *comment = '\0';
        }
        if (isBlankLine(buffer)) continue;

        if (buffer[0] == '\0' || buffer[0] == '\n') {
            continue;
        }

        if (buffer[0] == '.') {
            char label[MAX_LABEL_LENGTH];

            if (sscanf(buffer, "%49s", label) != 1) {
                continue;
            }

            if (!is_valid_label(label)) {
                printf("Invalid label: %s\n", label);
                fclose(input);
                fclose(output);
                return;
            }

            if (find_label(label) != -1) {
                printf("Duplicate label: %s\n", label);
                fclose(input);
                fclose(output);
                return;
            }

            if (label_count >= MAX_LABELS) {
                printf("Too many labels in program.\n");
                fclose(input);
                fclose(output);
                return NULL;
            }

            strcpy(labels[label_count].name, label);
            labels[label_count].instruction_index = instruction_index;
            label_count++;

            printf("  Label found: %s -> instruction %d\n", label, instruction_index);
            continue;
        }

        instruction_index++;
    }

    print_labels();

    /*
     * PASS 2:
     * Generate bytecode.
     */
    printf("Pass 2: generating bytecode...\n");
    rewind(input);
    instruction_index = 0;

    while (fgets(buffer, sizeof(buffer), input)) {
        char *comment = strchr(buffer, '%');

        if (comment != NULL) {
            *comment = '\0';
        }
        if (isBlankLine(buffer)) continue;
        if (buffer[0] == '\0' || buffer[0] == '\n' || buffer[0] == '.') {
            continue;
        }

        int dest, src1, src2, value, opcode;
        char op;

        printf("Compiling instruction %d: %s\n", instruction_index, buffer);

        if (strncmp(buffer, "Print", 5) == 0) {
            int reg_num = 0;
            
            // Parse "Print x<number>"
            if (sscanf(buffer, "Print x%d", &reg_num) == 1) {
                if (reg_num < 0 || reg_num >= 256) {
                    printf("Compile Error: Invalid register x%d\n", reg_num);
                    // Handle error / return
                } else {
                    // Format: Opcode (0x08) | Dest (0) | Src1 (0) | Src2 (reg_num)
                    fprintf(output, "%02X 0 0 %02X\n", OP_PRINT, reg_num);
                    instruction_index++;
                }
            } else {
                printf("Compile Error: Invalid Print syntax. Expected: Print x<reg>\n");
            }
            continue;
        }
        /*
         * Legacy READ:
         * Read x1, 10
         */
        if (strncmp(buffer, "Read", 4) == 0) {
            int reg, address;

            if (sscanf(buffer, "Read x%d, %d", &reg, &address) != 2) {
                printf("Invalid Read instruction: %s\n", buffer);
                continue;
            }

            if (!is_valid_register(reg) || !is_valid_constant(address)) {
                printf("Invalid Read operands: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_READ, reg, address);
            printf("  -> %02X %02X %02X 00\n", OP_READ, reg, address);
            instruction_index++;
            continue;
        }

        /*
         * Legacy WRITE:
         * Write x1, 10
         */
        if (strncmp(buffer, "Write", 5) == 0) {
            int reg, address;

            if (sscanf(buffer, "Write x%d, %d", &reg, &address) != 2) {
                printf("Invalid Write instruction: %s\n", buffer);
                continue;
            }

            if (!is_valid_register(reg) || !is_valid_constant(address)) {
                printf("Invalid Write operands: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_WRITE, reg, address);
            printf("  -> %02X %02X %02X 00\n", OP_WRITE, reg, address);
            instruction_index++;
            continue;
        }

        /*
         * Memory READ with register address:
         * x1 = [x2]
         */
        if (sscanf(buffer, "x%d = [x%d]", &dest, &src1) == 2) {
            if (!is_valid_register(dest) || !is_valid_register(src1)) {
                printf("Invalid register in memory read: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_READ, dest, src1);
            printf("  -> %02X %02X %02X 00\n", OP_READ, dest, src1);
            instruction_index++;
            continue;
        }

        /*
         * Memory READ with constant address:
         * x1 = [100]
         */
        if (sscanf(buffer, "x%d = [%d]", &dest, &value) == 2) {
            if (!is_valid_register(dest) || !is_valid_constant(value)) {
                printf("Invalid operand in memory read: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_READ_CONST, dest, value);
            printf("  -> %02X %02X %02X 00\n", OP_READ_CONST, dest, value);
            instruction_index++;
            continue;
        }

        /*
         * Memory WRITE with register address:
         * [x1] = x2
         */
        if (sscanf(buffer, "[x%d] = x%d", &dest, &src1) == 2) {
            if (!is_valid_register(dest) || !is_valid_register(src1)) {
                printf("Invalid register in memory write: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_WRITE, dest, src1);
            printf("  -> %02X %02X %02X 00\n", OP_WRITE, dest, src1);
            instruction_index++;
            continue;
        }

        /*
         * Memory WRITE with constant address:
         * [100] = x2
         */
        if (sscanf(buffer, "[%d] = x%d", &value, &src1) == 2) {
            if (!is_valid_constant(value) || !is_valid_register(src1)) {
                printf("Invalid operand in memory write: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_WRITE_CONST, src1, value);
            printf("  -> %02X %02X %02X 00\n", OP_WRITE_CONST, src1, value);
            instruction_index++;
            continue;
        }

        /*
            Read, Write on vectors
        */
        // Read vector with variable
        if (sscanf(buffer, "v%d = [x%d]", &dest, &src1) == 2) {
            if (!is_valid_vector_register(dest) || !is_valid_register(src1)) {
                printf("Invalid register in memory read: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_VEC_READ, dest, src1);
            printf("  -> %02X %02X %02X 00\n", OP_VEC_READ, dest, src1);
            instruction_index++;
            continue;
        }
        // Read vector with constant
        if (sscanf(buffer, "v%d = [%d]", &dest, &value) == 2) {
            if (!is_valid_vector_register(dest) || !is_valid_constant(value)) {
                printf("Invalid operand in memory read: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_VEC_READ_CONST, dest, value);
            printf("  -> %02X %02X %02X 00\n", OP_VEC_READ_CONST, dest, value);
            instruction_index++;
            continue;
        }
        // Write vector with variable
        if (sscanf(buffer, "[x%d] = v%d", &dest, &src1) == 2) {
            if (!is_valid_register(dest) || !is_valid_vector_register(src1)) {
                printf("Invalid register in memory write: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_VEC_WRITE, dest, src1);
            printf("  -> %02X %02X %02X 00\n", OP_VEC_WRITE, dest, src1);
            instruction_index++;
            continue;
        }
        // Write vector with constant
        if (sscanf(buffer, "[%d] = v%d", &value, &src1) == 2) {
            if (!is_valid_constant(value) || !is_valid_vector_register(src1)) {
                printf("Invalid operand in memory write: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_VEC_WRITE_CONST, src1, value);
            printf("  -> %02X %02X %02X 00\n", OP_VEC_WRITE_CONST, src1, value);
            instruction_index++;
            continue;
        }

        /*
         * Arithmetic with registers:
         * x1 = x2 + x3
         */
        if (sscanf(buffer, "x%d = x%d %c x%d", &dest, &src1, &op, &src2) == 4) {
            opcode = -1;

            if (op == '+') {
                opcode = OP_ADD;
            }
            else if (op == '-') {
                opcode = OP_SUB;
            }
            else if (op == '*') {
                opcode = OP_MUL;
            }
            else if (op == '/') {
                opcode = OP_DIV;
            }

            if (opcode == -1) {
                printf("Unknown operator %c\n", op);
                continue;
            }

            if (!is_valid_register(dest) || !is_valid_register(src1) || !is_valid_register(src2)) {
                printf("Invalid register in arithmetic: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X %X\n", opcode, dest, src1, src2);
            printf("  -> %02X %02X %02X %02X\n", opcode, dest, src1, src2);
            instruction_index++;
            continue;
        }

        /*
         * Arithmetic with constant:
         * x1 = x2 + 30
         */
        if (sscanf(buffer, "x%d = x%d %c %d", &dest, &src1, &op, &value) == 4) {
            opcode = -1;

            if (op == '+') {
                opcode = OP_ADD_CONST;
            }
            else if (op == '-') {
                opcode = OP_SUB_CONST;
            }
            else if (op == '*') {
                opcode = OP_MUL_CONST;
            }
            else if (op == '/') {
                opcode = OP_DIV_CONST;
            }

            if (opcode == -1) {
                printf("Unknown operator %c\n", op);
                continue;
            }

            if (!is_valid_register(dest) || !is_valid_register(src1) || !is_valid_constant(value)) {
                printf("Invalid operand in constant arithmetic: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X %X\n", opcode, dest, src1, value);
            printf("  -> %02X %02X %02X %02X\n", opcode, dest, src1, value);
            instruction_index++;
            continue;
        }

        /* 
            Vector Arithmetic with variables
        */
        if (sscanf(buffer, "v%d = v%d %c v%d", &dest, &src1, &op, &src2) == 4) {
            opcode = -1;

            if (op == '+') {
                opcode = OP_VEC_ADD;
            }
            else if (op == '-') {
                opcode = OP_VEC_SUB;
            }
            else if (op == '*') {
                opcode = OP_VEC_MUL;
            }

            if (opcode == -1) {
                printf("Unknown operator %c\n", op);
                continue;
            }

            if (!is_valid_vector_register(dest) || !is_valid_vector_register(src1) || !is_valid_vector_register(src2)) {
                printf("Invalid register in arithmetic: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X %X\n", opcode, dest, src1, src2);
            printf("  -> %02X %02X %02X %02X\n", opcode, dest, src1, src2);
            instruction_index++;
            continue;
        }

        /*
           Vector Arithmetic with constant 
        */
        if (sscanf(buffer, "v%d = v%d %c %d", &dest, &src1, &op, &value) == 4) {
            opcode = -1;

            if (op == '+') {
                opcode = OP_VEC_ADD_CONST;
            }
            else if (op == '-') {
                opcode = OP_VEC_SUB_CONST;
            }
            else if (op == '*') {
                opcode = OP_VEC_MUL_CONST;
            }

            if (opcode == -1) {
                printf("Unknown operator %c\n", op);
                continue;
            }

            if (!is_valid_vector_register(dest) || !is_valid_vector_register(src1) || !is_valid_vector_register(value)) {
                printf("Invalid operand in constant arithmetic: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X %X\n", opcode, dest, src1, value);
            printf("  -> %02X %02X %02X %02X\n", opcode, dest, src1, value);
            instruction_index++;
            continue;
        }

        /*
            Vector arithmetic with variable registers
        */
        if (sscanf(buffer, "v%d = v%d %c x%d", &dest, &src1, &op, &value) == 4) {
            opcode = -1;

            if (op == '+') {
                opcode = OP_VEC_ADD_REG;
            }
            else if (op == '-') {
                opcode = OP_VEC_SUB_REG;
            }
            else if (op == '*') {
                opcode = OP_VEC_MUL_REG;
            }

            if (opcode == -1) {
                printf("Unknown operator %c\n", op);
                continue;
            }

            if (!is_valid_vector_register(dest) || !is_valid_vector_register(src1) || !is_valid_register(value)) {
                printf("Invalid operand in constant arithmetic: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X %X\n", opcode, dest, src1, value);
            printf("  -> %02X %02X %02X %02X\n", opcode, dest, src1, value);
            instruction_index++;
            continue;
        }

        /*
         * Register movement:
         * x1 = x2
         */
        if (sscanf(buffer, "x%d = x%d", &dest, &src1) == 2) {
            if (!is_valid_register(dest) || !is_valid_register(src1)) {
                printf("Invalid register in MOV: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_MOV, dest, src1);
            printf("  -> %02X %02X %02X 00\n", OP_MOV, dest, src1);
            instruction_index++;
            continue;
        }

        /*
         * Constant movement:
         * x1 = 100
         */
        if (sscanf(buffer, "x%d = %d", &dest, &value) == 2) {
            if (!is_valid_register(dest) || !is_valid_constant(value)) {
                printf("Invalid operand in MOV_CONST: %s\n", buffer);
                continue;
            }

            fprintf(output, "%X %X %X 0\n", OP_MOV_CONST, dest, value);
            printf("  -> %02X %02X %02X 00\n", OP_MOV_CONST, dest, value);
            instruction_index++;
            continue;
        }

        {
            char branch[10];
            char label[MAX_LABEL_LENGTH];

            if (sscanf(buffer, "%9s %49s", branch, label) == 2) {
                int branch_opcode = get_branch_opcode(branch);

                if (branch_opcode != -1) {
                    int target = find_label(label);

                    if (target == -1) {
                        printf("Undefined label: %s\n", label);
                        continue;
                    }

                    int offset = target - instruction_index;

                    if (offset < -128 || offset > 127) {
                        printf("Branch offset out of range for label %s\n", label);
                        continue;
                    }

                    fprintf(output, "%X 0 0 %X\n", branch_opcode, offset & 0xFF);
                    printf("  -> %02X 00 00 %02X\n", branch_opcode, offset & 0xFF);
                    instruction_index++;
                    continue;
                }
            }
        }

        printf("Unable to compile instruction: %s\n", buffer);
    }

    fprintf(output, "0 0 0 0\n");
    printf("  -> 00 00 00 00 (HALT)\n");
    printf("Compilation complete. Wrote program.byte\n");

    fclose(input);
    fclose(output);

    return raw_filename;
}