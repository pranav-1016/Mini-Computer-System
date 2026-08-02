#include<stdio.h>
#include<string.h>
#include "compiler.h"
#include "memory.h"

void compile(const char *filename) {
    FILE *input = fopen(filename, "r");
    FILE *output = fopen("program.byte", "w");
    
    if (input == NULL) {
        printf("File not found or not opened >>>>>\n");
        return;
    }

    if (output == NULL) {
        printf("Unable to create program.byte >>>>>\n");
        return;
    }
    char buffer[100];
    while (fgets(buffer, 100, input)) {
        // buffer contains the content of each line
        
        // parse the tokens using sscanf
        if (strncmp(buffer, "Read", 4) == 0) {
            // Parse the read instruction
            int reg, address;
            sscanf(buffer, "Read x%d, %d", &reg, &address);

            fprintf(output, "5 %d %d 0\n", reg, address);


        } else if (strncmp(buffer, "Write", 5) == 0) {
            // Parse the write instruction
            int reg, address;
            sscanf(buffer, "Write x%d, %d", &reg, &address);

            fprintf(output, "6 %d %d 0\n", reg, address);

        } else {
            int dest, src1, src2, value, opcode;
            char op;
            
            // Artihmetic operation
            if (sscanf(buffer, "x%d = x%d %c x%d", &dest, &src1, &op, &src2) == 4) {
                if (op == '+') opcode = OP_ADD;
                else if (op == '-') opcode = OP_SUB;
                else if (op == '*') opcode = OP_MUL;
                else if (op == '/') opcode = OP_DIV;
                else printf("Unknown operator %c\n Failed to compile the instructions.\n", op);
                fprintf(output, "%d %d %d %d\n", opcode, dest, src1, src2);
            } else if (sscanf(buffer, "x%d = %d", &dest, &value) == 2){
                // data movement operation
                fprintf(output, "7 %d %d 0\n", dest, value);
            }
        }

    }
    fprintf(output, "0 0 0 0\n");    

    fclose(input);
    fclose(output);
    return;
}