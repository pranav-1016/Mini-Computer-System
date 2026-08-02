#include <stdio.h>
#include "memory.h"

unsigned char Instruction[MEM_SIZE] = {0};
int Data[MEM_SIZE] = {0};

void load_the_program() {
    FILE *program = fopen("program.byte", "r");
    if (program == NULL) {
        printf("program.byte not found or not opened >>>>>>\n");
        return;
    }
    printf("Loading program from the program.byte file ....\n");
    int idx = 0;
    int tmp;
    while (idx < MEM_SIZE && fscanf(program, "%d", &tmp) == 1) {
        Instruction[idx++] = (unsigned char) tmp;
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
    while (idx < MEM_SIZE && fscanf(data, "%d", &Data[idx]) == 1) {
        idx++;
    }
    fclose(data);
}

void initialise(const char *filename) {
    
    //read the data from “program.byte” and populate Instruction memory
    load_the_program();

    //read the data from “data.byte” and populate data memory. 
    load_the_data(filename);
}

void finalize(const char *filename) {
    printf("Code Executed and finally writing the data FILE %s \n", filename);
    FILE *data = fopen(filename, "w");
    if (data == NULL) {
        printf("Unable to open %s for writing >>>>>>\n", filename);
        return;
    }
    for (int i = 0; i < MEM_SIZE; i++) {
        fprintf(data, "%d\n", Data[i]);
    }
    fclose(data);
}
