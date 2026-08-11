#include <stdio.h>
#include "memory.h"

unsigned char Instruction[INSTRUCTION_MEM_SIZE] = {0};
int Data[DATA_MEM_SIZE] = {0};

void load_the_program() {
    FILE *program = fopen("program.byte", "r");

    if (program == NULL) {
        printf("program.byte not found or not opened >>>>>>\n");
        return;
    }

    printf("Loading program from the program.byte file ....\n");

    int idx = 0;
    unsigned int tmp;

    while (idx < INSTRUCTION_MEM_SIZE && fscanf(program, "%x", &tmp) == 1) {
        if (tmp > 0xFF) {
            printf("Invalid byte value in program.byte: %X\n", tmp);
            fclose(program);
            return;
        }

        Instruction[idx++] = (unsigned char)tmp;
    }

    fclose(program);
}

void load_the_data(const char *filename) {
    FILE *data = fopen(filename, "r");

    if (data == NULL) {
        printf("%s not found or not opened >>>>>>\n", filename);
        return;
    }

    printf("Loading data from the %s file ....\n", filename);

    int idx = 0;
    unsigned int tmp;

    while (idx < DATA_MEM_SIZE && fscanf(data, "%x", &tmp) == 1) {
        if (tmp > 0xFF) {
            printf("Invalid byte value in %s: %X\n", filename, tmp);
            fclose(data);
            return;
        }

        Data[idx++] = (int)tmp;
    }

    fclose(data);
}

void initialise(const char *filename) {
    // Read the data from "program.byte" and populate instruction memory
    load_the_program();

    // Read the data from "data.byte" and populate data memory
    load_the_data(filename);
}

void finalize(const char *filename) {
    printf("Code Executed and finally writing the data FILE %s\n", filename);

    FILE *data = fopen(filename, "w");

    if (data == NULL) {
        printf("Unable to open %s for writing >>>>>>\n", filename);
        return;
    }

    for (int i = 0; i < DATA_MEM_SIZE; i++) {
        fprintf(data, "%02X\n", (unsigned int)Data[i] & 0xFF);
    }

    fclose(data);
}