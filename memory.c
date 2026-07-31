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
    printf("Reading from the program.byte file ....\n");
    int idx = 0;
    int tmp;
    while (idx < MEM_SIZE && fscanf(program, "%d", &tmp) == 1) {
        Instruction[idx++] = (unsigned char) tmp;
    }
    fclose(program);
}

void load_the_data() {
    FILE *data = fopen("data.byte", "r");
    if (data == NULL) {
        printf("data.byte not found or not opened >>>>>>\n");
        return;
    }
    printf("Reading from the data.byte file ....\n");
    int idx = 0;
    while (idx < MEM_SIZE && fscanf(data, "%d", &Data[idx]) == 1) {
        idx++;
    }
    fclose(data);
}

void initialise() {
    
    //read the data from “program.byte” and populate Instruction memory
    load_the_program();

    //read the data from “data.byte” and populate data memory. 
    load_the_data();
}

void finalize() {
    printf("Code Executed and finally writing the data.byte FILE\n");
    FILE *data = fopen("data.byte", "w");
    if (data == NULL) {
        printf("Unable to open data.byte for writing >>>>>>\n");
        return;
    }
    for (int i = 0; i < MEM_SIZE; i++) {
        fprintf(data, "%d\n", Data[i]);
    }
    fclose(data);
}
