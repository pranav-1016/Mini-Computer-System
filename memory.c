#include<stdio.h>

unsigned char Instruction[256] = {0};
int Data[256] = {0};

// Assumes program.byte and data.byte present in the directory. The memory is byte addressable which means when you read instructions from char array will read the instruction bye-by-byte.
void load_the_program() {
    //read the data from “program.byte” and populate Instruction memory
    FILE *program = fopen("program.byte", "r");
    if (program == NULL) {
        printf("program.byte not found or not opened >>>>>>\n");
        fclose(program);
        return;
    }
    printf("Reading from the program.byte file ....\n");
    int idx = 0;
    while (fscanf(program, "%d", &Instruction[idx]) == 1) {
        idx++;
    }

    // Just for debug
    // for (int i=0; i < 256; i++) {
    //     printf("%d\n", Instruction[i]);
    // }


    fclose(program);
    return;
}

void load_the_data() {
    FILE *data = fopen("data.byte", "r");

    if (data == NULL) {
        printf("data.byte not found or opened >>>>>>\n");
        fclose(data);
        return;
    }
    printf("Reading from the data.byte file ....\n");
    int idx = 0;
    while (fscanf(data, "%d", &Data[idx]) == 1) {
        idx++;
    }

    // Just for debug
    // for (int i=0; i < 256; i++) {
    //     printf("%d\n", Data[i]);
    // }

    fclose(data);
    return;
}
void initialise() {
    
    //read the data from “program.byte” and populate Instruction memory
    load_the_program();

    //read the data from “data.byte” and populate data memory. 
    load_the_data();


}

void finalize() {
    printf("Code Executed and finally writing the data.byte FILE\n");
    //Write data.byte 
    FILE *data = fopen("data.byte", "w");
    if (data == NULL) {
        printf("data.byte not found or opened >>>>>>\n");
        return;
    }
    
    for (int i=0; i < 256; i++) {
        fprintf(data, "%d\n", Data[i]);
    }
    fclose(data);

    return;
} 
