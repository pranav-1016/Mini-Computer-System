#include <stdio.h>
#include <unistd.h>
#include "memory.h"
#include "os.h"

/* This was our runtime memory as previously */
// unsigned char Instruction[NP][INSTRUCTION_MEM_SIZE] = {{0}};
// int Data[NP][DATA_MEM_SIZE] = {{0}};

char memory[MEMSIZE] = {0};

static int io_input_value = 0;

#define CACHE_LINES 16
typedef struct {
    int valid;
    int physical_address;
    unsigned char value;
} CacheLine;

static CacheLine data_cache[NP][CACHE_LINES];
static unsigned long cache_hits[NP] = {0};
static unsigned long cache_misses[NP] = {0};

static int cache_read_byte(int proc_id, int physical_address) {
    int line_index = (physical_address / WORD_SIZE) % CACHE_LINES;
    CacheLine *line = &data_cache[proc_id][line_index];
    if (line->valid && line->physical_address == physical_address) {
        cache_hits[proc_id]++;
        usleep(10);
        return line->value;
    }

    cache_misses[proc_id]++;
    usleep(100);
    line->valid = 1;
    line->physical_address = physical_address;
    line->value = (unsigned char)memory[physical_address];
    return line->value;
}

static void cache_write_byte(int proc_id, int physical_address, unsigned char value) {
    int line_index = (physical_address / WORD_SIZE) % CACHE_LINES;
    CacheLine *line = &data_cache[proc_id][line_index];
    line->valid = 1;
    line->physical_address = physical_address;
    line->value = value;
    memory[physical_address] = (char)value;
}

static int read_io(int proc_id, int logical_address, int *value) {
    if (logical_address == IO_PROCESS_ID) {
        *value = proc_id;
        return 1;
    }
    if (logical_address == IO_CONSOLE_IN) {
        *value = io_input_value;
        return 1;
    }
    return 0;
}

static int write_io(int proc_id, int logical_address, int value) {
    if (logical_address == IO_CONSOLE_OUT) {
        printf("[MMIO PID %d] %d\n", proc_id, value);
        fflush(stdout);
        return 1;
    }
    if (logical_address == IO_CONSOLE_IN) {
        io_input_value = value;
        return 1;
    }
    return 0;
}

int read_word(int proc_id, int logical_address, int *value) {
    if (value == NULL) {
        log_system("Core %d: Cannot read 32-bit word: output value is NULL\n", proc_id);
        return 0;
    }

    if (proc_id < 0 || proc_id >= NP) {
        log_system("Invalid process id %d while reading 32-bit word\n", proc_id);
        return 0;
    }

    if (logical_address < 0 || logical_address > DATA_MEM_SIZE - WORD_SIZE) {
        if (logical_address == IO_PROCESS_ID || logical_address == IO_CONSOLE_IN) {
            return read_io(proc_id, logical_address, value);
        }
        log_system("Core %d: 32-bit read out of bounds (address=%d, word size=%d, data size=%d)\n",
                   proc_id, logical_address, WORD_SIZE, DATA_MEM_SIZE);
        return 0;
    }

    unsigned int result = 0;
    for (int byte_index = 0; byte_index < WORD_SIZE; byte_index++) {
        int physical_address = getPhysicalAddress(proc_id, ACCESS_READ, logical_address + byte_index);
        if (physical_address < 0 || physical_address >= MEMSIZE) {
            log_system("Core %d: 32-bit read translation failed (logical address=%d, physical address=%d)\n",
                       proc_id, logical_address + byte_index, physical_address);
            return 0;
        }
        result |= ((unsigned int)cache_read_byte(proc_id, physical_address)) << (byte_index * 8);
    }

    *value = (int)result;
    return 1;
}

int write_word(int proc_id, int logical_address, int value) {
    if (proc_id < 0 || proc_id >= NP) {
        log_system("Invalid process id %d while writing 32-bit word\n", proc_id);
        return 0;
    }

    if (logical_address < 0 || logical_address > DATA_MEM_SIZE - WORD_SIZE) {
        if (logical_address == IO_CONSOLE_OUT || logical_address == IO_CONSOLE_IN) {
            return write_io(proc_id, logical_address, value);
        }
        log_system("Core %d: 32-bit write out of bounds (address=%d, word size=%d, data size=%d)\n",
                   proc_id, logical_address, WORD_SIZE, DATA_MEM_SIZE);
        return 0;
    }

    int physical_addresses[WORD_SIZE];
    for (int byte_index = 0; byte_index < WORD_SIZE; byte_index++) {
        physical_addresses[byte_index] = getPhysicalAddress(proc_id, ACCESS_WRITE, logical_address + byte_index);
        if (physical_addresses[byte_index] < 0 || physical_addresses[byte_index] >= MEMSIZE) {
            log_system("Core %d: 32-bit write translation failed (logical address=%d, physical address=%d)\n",
                       proc_id, logical_address + byte_index, physical_addresses[byte_index]);
            return 0;
        }
    }

    for (int byte_index = 0; byte_index < WORD_SIZE; byte_index++) {
        cache_write_byte(proc_id, physical_addresses[byte_index],
                 (unsigned char)(((unsigned int)value >> (byte_index * 8)) & 0xFF));
    }

    return 1;
}

void print_memory_statistics(int proc_id) {
    if (proc_id < 0 || proc_id >= NP) return;
    printf("PID %d cache hits=%lu misses=%lu\n", proc_id,
           cache_hits[proc_id], cache_misses[proc_id]);
}


// void load_the_program(int proc_id, const char *filename) {
//     if (proc_id < 0 || proc_id >= NP) return;

//     FILE *program = fopen(filename, "r");

//     if (program == NULL) {
//         log_system("Core %d: %s not found or not opened >>>>>>\n", proc_id, filename);
//         return;
//     }

//     log_system("Core %d: Loading program from %s ....\n", proc_id, filename);

//     int idx = 0;
//     unsigned int tmp;

//     while (idx < INSTRUCTION_MEM_SIZE && fscanf(program, "%x", &tmp) == 1) {
//         if (tmp > 0xFF) {
//             log_system("Core %d: Invalid byte value in %s: %X\n", proc_id, filename, tmp);
//             fclose(program);
//             return;
//         }

//         Instruction[proc_id][idx++] = (unsigned char)tmp;
//     }

//     fclose(program);
// }

// void load_the_data(int proc_id, const char *filename) {
//     if (proc_id < 0 || proc_id >= NP) return;

//     FILE *data = fopen(filename, "r");

//     if (data == NULL) {
//         log_system("Core %d: %s not found or not opened >>>>>>\n", proc_id, filename);
//         return;
//     }

//     log_system("Core %d: Loading data from %s ....\n", proc_id, filename);

//     int idx = 0;
//     unsigned int tmp;

//     while (idx < DATA_MEM_SIZE && fscanf(data, "%x", &tmp) == 1) {
//         if (tmp > 0xFF) {
//             log_system("Core %d: Invalid byte value in %s: %X\n", proc_id, filename, tmp);
//             fclose(data);
//             return;
//         }

//         Data[proc_id][idx++] = (int)tmp;
//     }

//     fclose(data);
// }

// void initialise(int proc_id, const char *program_file, const char *data_file) {
//     if (proc_id < 0 || proc_id >= NP) {
//         log_system("Invalid process id\n");
//         return;
//     }

//     load_the_program(proc_id, program_file);
//     load_the_data(proc_id, data_file);
// }

// void finalize(int proc_id, const char *filename) {
    //Write data.byte 
    //reset page table. 
    //Free pages from freePage array. 

//     if (proc_id < 0 || proc_id >= NP) {
//         log_system("Invalid process id\n");
//         return;
//     }

//     log_system("Core %d: Code Executed, writing data FILE %s\n", proc_id, filename);

//     FILE *data = fopen(filename, "w");

//     if (data == NULL) {
//         log_system("Core %d: Unable to open %s for writing >>>>>>\n", proc_id, filename);
//         return;
//     }

//     for (int i = 0; i < DATA_MEM_SIZE; i += 4) {
//         fprintf(data, "%02X %02X %02X %02X\n",
//                 (unsigned int)Data[proc_id][i] & 0xFF,
//                 (i + 1 < DATA_MEM_SIZE) ? (unsigned int)Data[proc_id][i + 1] & 0xFF : 0,
//                 (i + 2 < DATA_MEM_SIZE) ? (unsigned int)Data[proc_id][i + 2] & 0xFF : 0,
//                 (i + 3 < DATA_MEM_SIZE) ? (unsigned int)Data[proc_id][i + 3] & 0xFF : 0);
//     }

//     fclose(data);
// }