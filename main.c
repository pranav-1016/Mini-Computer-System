#include<stdio.h>
#include "compiler.h"
#include "memory.h"
#include "processor.h"

int main(int argc, char *argv[]) {
    const char *input_filename = "input.txt";
    const char *data_filename = "data.byte";

    
    if (argc == 3) {
        input_filename = argv[1];
        data_filename = argv[2];
    } else if (argc != 1) {
        printf("Either use \"./simulator.exe [input_filename] [data_filename]\" or \n");
        printf("Use default \"./simulator.exe\", with default input.txt and data.byte \n");
        
        return 1;
    }
    printf("Input program code : %s\n" , input_filename);
    printf("Data file name: %s\n", data_filename);

    compile(input_filename);
    initialise(data_filename);
    reset();
    extern int end_of_simulation;
    while(!end_of_simulation) {
        fetch();
        decode();
        execute();

    }

    finalize(data_filename);
    return 0;
}
    