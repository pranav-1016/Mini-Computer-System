#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static char input_buffer[256];
static int buf_idx = 0;
static struct termios orig_termios;

int processor_busy[NP] = {0}; // Maps processor availability[cite: 1]
int next_pid = 1;             // Unique PID generator[cite: 1]
int shell_active = 1;         // Flag to disable shell input on "exit"[cite: 1]

// Finds an available hardware processor ID (0 to NP-1)[cite: 1]
int allocate_processor(void) {
    for (int i = 0; i < NP; i++) {
        if (!processor_busy[i]) {
            processor_busy[i] = 1;
            return i;
        }
    }
    return -1;
}

// Configures STDIN for non-blocking & unbuffered input[cite: 1]
void init_terminal(void) {
    struct termios new_termios;

    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;

    // Disable canonical mode (line buffering) and automatic input echo
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    // Set STDIN to non-blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // Initialize Queues
    initQueue(&readyQueue);
    initQueue(&waitQueue);
    
    // Print initial prompt
    printf("Mini-Computer simulator is ready >>> $ ");
    fflush(stdout);
}

// Restores original terminal settings on exit
void reset_keyboard(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// Round-Robin OS Scheduler[cite: 1]
void scheduler(void) {
    int count = readyQueue.size;

    // 1. Time-slice each ready task (10 instruction cycles per process)[cite: 1]
    for (int i = 0; i < count; i++) {
        PCB process;
        if (!dequeue(&readyQueue, &process)) break;

        process.state = PROCESS_RUNNING;
        
        // Execute 10 instructions on this process's assigned processor[cite: 1]
        process_instructions(process.proc_id, 10);

        // Check if process finished execution (opcode 0x00)[cite: 1]
        if (end_of_simulation[process.proc_id]) {
            finalize(process.proc_id, process.data_file); // Save memory state[cite: 1]
            processor_busy[process.proc_id] = 0;          // Free processor[cite: 1]

            // Promote waiting process from waitQueue if available[cite: 1]
            PCB waiting_proc;
            if (dequeue(&waitQueue, &waiting_proc)) {
                loader(waiting_proc.program_file, waiting_proc.data_file);
            }
        } else {
            // Re-queue process if not finished[cite: 1]
            process.state = PROCESS_READY;
            enqueue(&readyQueue, process);
        }
    }

    // 2. Poll non-blocking shell once per cycle if active[cite: 1]
    if (shell_active) {
        shell();
    }
}

// System Loader: Compiles, maps processor ID, and initializes memory[cite: 1, 4]
void loader(const char *program_file, const char *data_file) {
    if (!program_file || !data_file) {
        printf("Wrong command! Usage: [program_filename] [data_filename]\n");
        return;
    }

    PCB pcb;
    pcb.pid = next_pid++;
    strncpy(pcb.program_file, program_file, PATH_LENGTH - 1);
    pcb.program_file[PATH_LENGTH - 1] = '\0';
    strncpy(pcb.data_file, data_file, PATH_LENGTH - 1);
    pcb.data_file[PATH_LENGTH - 1] = '\0';

    // Copy string before calling compile() because strtok() mutates input[cite: 4]
    char temp_prog_name[PATH_LENGTH];
    strncpy(temp_prog_name, program_file, PATH_LENGTH - 1);
    temp_prog_name[PATH_LENGTH - 1] = '\0';

    // 1. Compile source program into bytecode[cite: 1, 4]
    printf("Now will call the compile method .....\n");
    char *compiled_byte_file = compile(temp_prog_name);
    if (compiled_byte_file == NULL) {
        printf("Compilation failed for %s\n", program_file);
        return;
    }
    printf("Compilation completed successfully <<<<<< %s", compiled_byte_file);

    // 2. Assign free processor if available[cite: 1]
    int free_proc = allocate_processor();

    if (free_proc != -1) {
        pcb.proc_id = free_proc;
        pcb.state = PROCESS_READY;

        // Initialize processor memory & state for proc_id[cite: 1]
        initialise(pcb.proc_id, compiled_byte_file, pcb.data_file);
        reset(pcb.proc_id);

        enqueue(&readyQueue, pcb);
    } else {
        // All NP processors busy -> place in wait Queue[cite: 1]
        pcb.proc_id = -1;
        pcb.state = PROCESS_READY;
        printf("Process enqueued in the queue");
        enqueue(&waitQueue, pcb);
    }
}

// Non-blocking Shell implementation[cite: 1]
void shell(void) {
    char ch;
    
    // Read available characters from STDIN without blocking[cite: 1]
    while (read(STDIN_FILENO, &ch, 1) > 0) {
        
        // Scenario A: User presses Enter[cite: 1]
        if (ch == '\n' || ch == '\r') {
            input_buffer[buf_idx] = '\0';
            printf("Enter pressed and processing the input\n"); 
            printf("Runing the code till here 1.\n");
            
            if (strlen(input_buffer) > 0) {
                printf("Runing the code till here 2(checkiong if buffer is exit).\n");
                if (strcmp(input_buffer, "exit") == 0) {
                    printf("Runing the code till here 3 if block.\n");
                    shell_active = 0; // Stop receiving shell input[cite: 1]
                    printf("Shell disabled. Finishing running tasks...\n");
                } else {
                    printf("Runing the code till here 4. else block.\n");
                    char prog_file[PATH_LENGTH], data_file[PATH_LENGTH];
                    if (sscanf(input_buffer, "%s %s", prog_file, data_file) == 2) {
                        printf("Runing the code till here. running the loader\n");
                        loader(prog_file, data_file); // Trigger loader[cite: 1]
                    } else {
                        printf("Invalid Command format. Use: <program.txt> <data.byte>\n");
                    }
                }
            }
            
            buf_idx = 0;
            if (shell_active) {
                printf("$ ");
                fflush(stdout);
            }
        } 
        // Scenario B: User presses Backspace
        else if (ch == 127 || ch == '\b') {
            if (buf_idx > 0) {
                buf_idx--;
                printf("\b \b");
                fflush(stdout);
            }
        } 
        // Scenario C: Standard printable characters[cite: 1]
        else if (buf_idx < (int)sizeof(input_buffer) - 1) {
            input_buffer[buf_idx++] = ch;
            putchar(ch);
            fflush(stdout);
        }
    }
}