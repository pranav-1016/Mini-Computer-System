#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include "compiler.h"
#include "memory.h"
#include "queue.h"
#include "processor.h"
#include "os.h"

CircularQueue readyQueue;
CircularQueue waitQueue;

FILE *fd_log = NULL;
FILE *fd_system_log = NULL;

static char input_buffer[256];
static int buf_idx = 0;
static struct termios orig_termios;

int processor_busy[NP] = {0}; 
int next_pid = 1;             
int shell_active = 1;         

// Page table mapping for each process: pageTable[proc_id][logical_page_index]
char pageTable[MAX_PROC][NUM_LOGICAL_PAGES];

// Physical frame availability tracker: 0 = Free, 1 = Allocated
char freePages[NUM_PHYSICAL_PAGES] = {0};

int allocate_processor(void) {
    for (int i = 0; i < NP; i++) {
        if (!processor_busy[i]) {
            processor_busy[i] = 1;
            return i;
        }
    }
    return -1;
}

void init_terminal(void) {
    struct termios new_termios;

    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;

    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    initQueue(&readyQueue);
    initQueue(&waitQueue);
    
    printf("Mini-Computer simulator is ready >>> $ ");
    fflush(stdout);
}

void init_system_logs(void) {
    if (fd_log == NULL) {
        fd_log = fopen("simulator.log", "w");
    }
    if (fd_system_log == NULL) {
        fd_system_log = fopen("system.log", "w");
    }
}

void log_system(const char *format, ...) {
    if (fd_system_log != NULL) {
        va_list args;
        va_start(args, format);
        vfprintf(fd_system_log, format, args);
        va_end(args);
        fflush(fd_system_log);
    }
}

void reset_keyboard(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void cleanup_system(void) {
    if (fd_log != NULL) {
        fclose(fd_log);
        fd_log = NULL;
    }
    if (fd_system_log != NULL) {
        fclose(fd_system_log);
        fd_system_log = NULL;
    }
    reset_keyboard();
}
void init_page_table(int proc_id) {
    for (int page = 0; page < NUM_LOGICAL_PAGES; page++) {
        pageTable[proc_id][page] = -1; // -1 means page is not loaded / unmapped
    }
}

int getPhysicalAddress(int proc_id, int isFetch, int address) {
    int page_idx = (isFetch) ? (address / PAGESIZE) : ((1024 / PAGESIZE) + (address / PAGESIZE));

    // Bounds/Unmapped Check
    if (page_idx < 0 || page_idx >= NUM_LOGICAL_PAGES || pageTable[proc_id][page_idx] == -1) {
        log_system("[PAGE FAULT] PID %d accessed unmapped logical page %d (Addr: %d)\n", 
                   proc_id, page_idx, address);
        kill_process(proc_id);
        return -1;
    }

    int frame = pageTable[proc_id][page_idx];
    return (frame * PAGESIZE) + (address % PAGESIZE);
}

void kill_process(int proc_id) {
    log_system("[PROCESS TERMINATED] PID %d killed due to invalid memory access.\n", proc_id);
    end_of_simulation[proc_id] = 1;               
}
int getFreePage(void) {
    // Frame 0 is reserved by the OS and is never allocated
    for (int i = 1; i < NUM_PHYSICAL_PAGES; i++) {
        if (freePages[i] == 0) {
            freePages[i] = 1; // Mark frame as allocated
            printf("Free page no. %d returned \n", i);
            return i;
        }
    }
    log_system("[ERROR] Out of physical memory! No free frames available.\n");
    printf("[ERROR] Out of physical memory! No free frames available.\n");
    exit(1);
}

void freeProcessPages(int proc_id) {
    for (int i = 0; i < NUM_LOGICAL_PAGES; i++) {
        int frame = (unsigned char)pageTable[proc_id][i];
        if (frame > 0 && frame < NUM_PHYSICAL_PAGES) {
            freePages[frame] = 0; // Unmark frame in free tracking array
        }
        pageTable[proc_id][i] = 0; // Clear entry in page table
    }
}

/* Loads bytecode directly into frame-allocated physical memory */
static int load_bytes_to_memory(const char *program_file, const char *data_file, int proc_id) {
    FILE *fprog = fopen(program_file, "r");
    FILE *fdata = fopen(data_file, "r");

    if (!fprog || !fdata) {
        if (fprog) fclose(fprog);
        if (fdata) fclose(fdata);
        return 0;
    }

    // Reset page table to unmapped (-1)
    init_page_table(proc_id);

    unsigned int bytes[4];
    int inst_byte_count = 0;

    // 1. Load Instruction Memory Pages
    while (fscanf(fprog, "%x %x %x %x", &bytes[0], &bytes[1], &bytes[2], &bytes[3]) == 4) {
        int page_idx = inst_byte_count / PAGESIZE;

        // Allocate physical frame if page is unmapped
        if (pageTable[proc_id][page_idx] == -1) {
            int frame = getFreePage();
            if (frame == -1) { fclose(fprog); fclose(fdata); return 0; }
            pageTable[proc_id][page_idx] = frame;
        }

        int phys_addr = (pageTable[proc_id][page_idx] * PAGESIZE) + (inst_byte_count % PAGESIZE);
        for (int b = 0; b < 4; b++) {
            memory[phys_addr + b] = (char)bytes[b];
        }
        inst_byte_count += 4;
    }
    fclose(fprog);

    // 2. Load Data Memory Pages (Up to Page Boundary of FF FF FF FF)
    int data_byte_count = 0;
    int data_page_offset = 1024 / PAGESIZE; // Data pages start after instructions
    int stop_after_current_page = 0;

    while (fscanf(fdata, "%x %x %x %x", &bytes[0], &bytes[1], &bytes[2], &bytes[3]) == 4) {
        int logical_page_idx = data_page_offset + (data_byte_count / PAGESIZE);

        // If we hit sentinel in a previous page and crossed boundary, terminate loading
        if (stop_after_current_page && (data_byte_count % PAGESIZE == 0)) {
            break;
        }

        // Allocate physical frame for new data page
        if (pageTable[proc_id][logical_page_idx] == -1) {
            int frame = getFreePage();
            if (frame == -1) { fclose(fdata); return 0; }
            pageTable[proc_id][logical_page_idx] = frame;
        }

        int phys_addr = (pageTable[proc_id][logical_page_idx] * PAGESIZE) + (data_byte_count % PAGESIZE);
        for (int b = 0; b < 4; b++) {
            memory[phys_addr + b] = (char)bytes[b];
        }

        // Check if FF FF FF FF sentinel is encountered
        if (bytes[0] == 0xFF && bytes[1] == 0xFF && bytes[2] == 0xFF && bytes[3] == 0xFF) {
            stop_after_current_page = 1; // Allow current page to complete, but stop at next page boundary
        }

        data_byte_count += 4;
    }

    fclose(fdata);
    return 1;
}


void initialise(int proc_id, const char *program_file, const char *data_file) {
    if (proc_id < 0 || proc_id >= MAX_PROC) {
        log_system("Invalid process id\n");
        return;
    }
    load_bytes_to_memory(program_file, data_file, proc_id);
}

void finalize(int proc_id, const char *output_data_file) {
    if (proc_id < 0 || proc_id >= MAX_PROC) {
        log_system("Invalid process id\n");
        return;
    }

    log_system("Core %d: Code Executed, writing data FILE %s\n", proc_id, output_data_file);

    FILE *fdata = fopen(output_data_file, "w");
    int data_page_offset = 1024 / PAGESIZE;

    if (fdata != NULL) {
        // Iterate through all logical data pages (pages 2 to 9)
        for (int page = data_page_offset; page < NUM_LOGICAL_PAGES; page++) {
            int frame = pageTable[proc_id][page];
            
            // Only write back pages that were allocated/loaded
            if (frame != -1) {
                int frame_start = frame * PAGESIZE;

                for (int offset = 0; offset < PAGESIZE; offset += 4) {
                    fprintf(fdata, "%02X %02X %02X %02X\n",
                            (unsigned char)memory[frame_start + offset],
                            (unsigned char)memory[frame_start + offset + 1],
                            (unsigned char)memory[frame_start + offset + 2],
                            (unsigned char)memory[frame_start + offset + 3]);
                }
            }
        }
        fclose(fdata);
    }

    // Free allocated frames back to OS pool and reset page table
    freeProcessPages(proc_id);
}

void loader(const char *program_file, const char *data_file) {
    if (!program_file || !data_file) {
        log_system("Wrong command! Usage: [program_filename] [data_filename]\n");
        return;
    }

    PCB pcb;
    pcb.pid = next_pid++;
    strncpy(pcb.program_file, program_file, PATH_LENGTH - 1);
    pcb.program_file[PATH_LENGTH - 1] = '\0';
    strncpy(pcb.data_file, data_file, PATH_LENGTH - 1);
    pcb.data_file[PATH_LENGTH - 1] = '\0';

    char temp_prog_name[PATH_LENGTH];
    strncpy(temp_prog_name, program_file, PATH_LENGTH - 1);
    temp_prog_name[PATH_LENGTH - 1] = '\0';

    char *compiled_byte_file = compile(temp_prog_name);
    if (compiled_byte_file == NULL) {
        log_system("Compilation failed for %s\n", program_file);
        return;
    }
    log_system("Compilation completed successfully <<<<<< %s\n", compiled_byte_file);

    int free_proc = allocate_processor();

    if (free_proc != -1) {
        pcb.proc_id = free_proc;
        pcb.state = PROCESS_READY;

        initialise(pcb.proc_id, compiled_byte_file, pcb.data_file);
        reset(pcb.proc_id);

        enqueue(&readyQueue, pcb);
        printf("[OS] Loaded program '%s' into Core %d (PID: %d)\n", program_file, pcb.proc_id, pcb.pid);
    } else {
        pcb.proc_id = -1;
        pcb.state = PROCESS_READY;
        log_system("Process ID %d placed in wait queue\n", pcb.pid);
        enqueue(&waitQueue, pcb);
        printf("[OS] Placed program '%s' into wait queue (PID: %d)\n", program_file, pcb.pid);
    }
}

void scheduler(void) {
    int count = readyQueue.size;

    for (int i = 0; i < count; i++) {
        PCB process;
        if (!dequeue(&readyQueue, &process)) break;

        process.state = PROCESS_RUNNING;
        
        process_instructions(process.proc_id, BURST_TIME);

        if (end_of_simulation[process.proc_id]) {
            printf("[OS] Process PID %d on Core %d finished execution.\n", process.pid, process.proc_id);
            
            finalize(process.proc_id, process.data_file); 
            processor_busy[process.proc_id] = 0;          

            PCB waiting_proc;
            if (dequeue(&waitQueue, &waiting_proc)) {
                loader(waiting_proc.program_file, waiting_proc.data_file);
            }
        } else {
            process.state = PROCESS_READY;
            enqueue(&readyQueue, process);
        }
    }

    if (shell_active) {
        shell();
    }
}

void shell(void) {
    char ch;
    
    while (read(STDIN_FILENO, &ch, 1) > 0) {
        
        if (ch == '\n' || ch == '\r') {
            input_buffer[buf_idx] = '\0';
            printf("\n");
            
            if (strlen(input_buffer) > 0) {
                if (strcmp(input_buffer, "exit") == 0) {
                    shell_active = 0; 
                } else {
                    char prog_file[PATH_LENGTH], data_file[PATH_LENGTH];
                    if (sscanf(input_buffer, "%s %s", prog_file, data_file) == 2) {
                        loader(prog_file, data_file); 
                    } else {
                        log_system("Invalid Command format. Use: <program.txt> <data.byte>\n");
                    }
                }
            }
            
            buf_idx = 0;
            if (shell_active) {
                printf("$ ");
                fflush(stdout);
            }
        } 
        else if (ch == 127 || ch == '\b') {
            if (buf_idx > 0) {
                buf_idx--;
                printf("\b \b");
                fflush(stdout);
            }
        } 
        else if (buf_idx < (int)sizeof(input_buffer) - 1) {
            input_buffer[buf_idx++] = ch;
            putchar(ch);
            fflush(stdout);
        }
    }
}