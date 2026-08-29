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
        reset(pcb.proc_id); // Make sure this sets end_of_simulation[proc_id] = 0!

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