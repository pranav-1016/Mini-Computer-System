#include <stdio.h>
#include "memory.h"
#include "os.h"

unsigned char Instruction[NP][INSTRUCTION_MEM_SIZE] = {{0}};
int Data[NP][DATA_MEM_SIZE] = {{0}};

void load_the_program(int proc_id, const char *filename) {
    if (proc_id < 0 || proc_id >= NP) return;

    FILE *program = fopen(filename, "r");

    if (program == NULL) {
        log_system("Core %d: %s not found or not opened >>>>>>\n", proc_id, filename);
        return;
    }

    log_system("Core %d: Loading program from %s ....\n", proc_id, filename);

    int idx = 0;
    unsigned int tmp;

    while (idx < INSTRUCTION_MEM_SIZE && fscanf(program, "%x", &tmp) == 1) {
        if (tmp > 0xFF) {
            log_system("Core %d: Invalid byte value in %s: %X\n", proc_id, filename, tmp);
            fclose(program);
            return;
        }

        Instruction[proc_id][idx++] = (unsigned char)tmp;
    }

    fclose(program);
}

void load_the_data(int proc_id, const char *filename) {
    if (proc_id < 0 || proc_id >= NP) return;

    FILE *data = fopen(filename, "r");

    if (data == NULL) {
        log_system("Core %d: %s not found or not opened >>>>>>\n", proc_id, filename);
        return;
    }

    log_system("Core %d: Loading data from %s ....\n", proc_id, filename);

    int idx = 0;
    unsigned int tmp;

    while (idx < DATA_MEM_SIZE && fscanf(data, "%x", &tmp) == 1) {
        if (tmp > 0xFF) {
            log_system("Core %d: Invalid byte value in %s: %X\n", proc_id, filename, tmp);
            fclose(data);
            return;
        }

        Data[proc_id][idx++] = (int)tmp;
    }

    fclose(data);
}

void initialise(int proc_id, const char *program_file, const char *data_file) {
    if (proc_id < 0 || proc_id >= NP) {
        log_system("Invalid process id\n");
        return;
    }

    load_the_program(proc_id, program_file);
    load_the_data(proc_id, data_file);
}

void finalize(int proc_id, const char *filename) {
    if (proc_id < 0 || proc_id >= NP) {
        log_system("Invalid process id\n");
        return;
    }

    log_system("Core %d: Code Executed, writing data FILE %s\n", proc_id, filename);

    FILE *data = fopen(filename, "w");

    if (data == NULL) {
        log_system("Core %d: Unable to open %s for writing >>>>>>\n", proc_id, filename);
        return;
    }

    for (int i = 0; i < DATA_MEM_SIZE; i += 4) {
        fprintf(data, "%02X %02X %02X %02X\n",
                (unsigned int)Data[proc_id][i] & 0xFF,
                (i + 1 < DATA_MEM_SIZE) ? (unsigned int)Data[proc_id][i + 1] & 0xFF : 0,
                (i + 2 < DATA_MEM_SIZE) ? (unsigned int)Data[proc_id][i + 2] & 0xFF : 0,
                (i + 3 < DATA_MEM_SIZE) ? (unsigned int)Data[proc_id][i + 3] & 0xFF : 0);
    }

    fclose(data);
}