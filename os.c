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

typedef struct {
    int valid;
    int logical_page;
    int frame;
    int permissions;
} TLBEntry;

static int pageTableFrame[MAX_PROC] = {-1, -1, -1, -1};
static TLBEntry tlb[MAX_PROC][TLB_ENTRIES];
static unsigned long tlb_hits[MAX_PROC] = {0};
static unsigned long tlb_misses[MAX_PROC] = {0};
static unsigned int tlb_next[MAX_PROC] = {0};
static int shared_memory_frame = -1;

// Physical frame availability tracker: 0 = Free, 1 = Allocated
char freePages[NUM_PHYSICAL_PAGES] = {0};

void printPageAllocation(int proc_id) {
    printf("--------------------------------------\n");
    printf("Page Table for process id %d :\n", proc_id);
    for (int i=0; i < NUM_LOGICAL_PAGES; i++) {
        int entry = get_page_entry(proc_id, i);
        if (entry & PTE_VALID) {
            printf("Page %d : Frame %d Permissions 0x%X\n", i,
                   entry >> PTE_FRAME_SHIFT, entry & 0xFF);
        }
    }
    printf("--------------------------------------\n");
}

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
static int page_table_physical_address(int proc_id, int logical_page) {
    if (proc_id < 0 || proc_id >= MAX_PROC || logical_page < 0 ||
        logical_page >= NUM_LOGICAL_PAGES || pageTableFrame[proc_id] < 0) {
        return -1;
    }
    return pageTableFrame[proc_id] * PAGESIZE + logical_page * (int)sizeof(int);
}

int get_page_entry(int proc_id, int logical_page) {
    int address = page_table_physical_address(proc_id, logical_page);
    if (address < 0 || address + (int)sizeof(int) > MEMSIZE) return 0;

    int entry = 0;
    for (int byte_index = 0; byte_index < (int)sizeof(int); byte_index++) {
        entry |= ((unsigned char)memory[address + byte_index]) << (byte_index * 8);
    }
    return entry;
}

int set_page_entry(int proc_id, int logical_page, int frame, int permissions) {
    int address = page_table_physical_address(proc_id, logical_page);
    if (address < 0 || address + (int)sizeof(int) > MEMSIZE) return 0;

        unsigned int entry = permissions == 0
                ? 0
                : ((unsigned int)frame << PTE_FRAME_SHIFT) |
                    (unsigned int)(PTE_VALID | permissions);
    for (int byte_index = 0; byte_index < (int)sizeof(int); byte_index++) {
        memory[address + byte_index] = (char)((entry >> (byte_index * 8)) & 0xFF);
    }
    return 1;
}

static void invalidate_tlb(int proc_id) {
    if (proc_id < 0 || proc_id >= MAX_PROC) return;
    for (int i = 0; i < TLB_ENTRIES; i++) tlb[proc_id][i].valid = 0;
    tlb_next[proc_id] = 0;
}

static int permission_for_access(int access_type) {
    if (access_type == ACCESS_EXECUTE) return PTE_EXECUTE;
    if (access_type == ACCESS_WRITE) return PTE_WRITE;
    return PTE_READ;
}

void kill_process(int proc_id);

void init_page_table(int proc_id) {
    if (proc_id < 0 || proc_id >= MAX_PROC) return;
    if (pageTableFrame[proc_id] < 0) pageTableFrame[proc_id] = getFreePage();
    for (int page = 0; page < NUM_LOGICAL_PAGES; page++) {
        set_page_entry(proc_id, page, 0, 0);
    }
    invalidate_tlb(proc_id);
}

int getPhysicalAddress(int proc_id, int access_type, int address) {
    if (proc_id < 0 || proc_id >= MAX_PROC || address < 0) {
        log_system("[MMU FAULT] Invalid access: process=%d address=%d\n", proc_id, address);
        if (proc_id >= 0 && proc_id < MAX_PROC) kill_process(proc_id);
        return -1;
    }

    int page_idx = (access_type == ACCESS_EXECUTE)
        ? (address / PAGESIZE)
        : ((1024 / PAGESIZE) + (address / PAGESIZE));
    int requested_permission = permission_for_access(access_type);

    for (int i = 0; i < TLB_ENTRIES; i++) {
        TLBEntry *entry = &tlb[proc_id][i];
        if (entry->valid && entry->logical_page == page_idx) {
            if ((entry->permissions & requested_permission) == 0) {
                log_system("[PROTECTION FAULT] PID %d page %d access=%d\n", proc_id, page_idx, access_type);
                kill_process(proc_id);
                return -1;
            }
            tlb_hits[proc_id]++;
            return entry->frame * PAGESIZE + (address % PAGESIZE);
        }
    }
    tlb_misses[proc_id]++;

    int page_entry = get_page_entry(proc_id, page_idx);
    if (page_idx < 0 || page_idx >= NUM_LOGICAL_PAGES || !(page_entry & PTE_VALID)) {
        log_system("[PAGE FAULT] PID %d accessed unmapped logical page %d (Addr: %d)\n",
                   proc_id, page_idx, address);
        kill_process(proc_id);
        return -1;
    }

    int permissions = page_entry & 0xFF;
    if ((permissions & requested_permission) == 0) {
        log_system("[PROTECTION FAULT] PID %d page %d access=%d\n", proc_id, page_idx, access_type);
        kill_process(proc_id);
        return -1;
    }

    int frame = page_entry >> PTE_FRAME_SHIFT;
    tlb[proc_id][tlb_next[proc_id]].valid = 1;
    tlb[proc_id][tlb_next[proc_id]].logical_page = page_idx;
    tlb[proc_id][tlb_next[proc_id]].frame = frame;
    tlb[proc_id][tlb_next[proc_id]].permissions = permissions;
    tlb_next[proc_id] = (tlb_next[proc_id] + 1) % TLB_ENTRIES;
    return frame * PAGESIZE + (address % PAGESIZE);
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
            // printf("Free page no. %d returned \n", i);
            return i;
        }
    }
    log_system("[ERROR] Out of physical memory! No free frames available.\n");
    printf("[ERROR] Out of physical memory! No free frames available.\n");
    exit(1);
}

void freeProcessPages(int proc_id) {
    for (int i = 0; i < NUM_LOGICAL_PAGES; i++) {
        int entry = get_page_entry(proc_id, i);
        int frame = entry >> PTE_FRAME_SHIFT;
        int shared_page = (1024 + SHARED_MEMORY_ADDRESS) / PAGESIZE;
        if (i == shared_page) continue;
        if (frame > 0 && frame < NUM_PHYSICAL_PAGES) {
            freePages[frame] = 0; // Unmark frame in free tracking array
        }
        set_page_entry(proc_id, i, 0, 0);
    }
    if (pageTableFrame[proc_id] > 0 && pageTableFrame[proc_id] < NUM_PHYSICAL_PAGES) {
        freePages[pageTableFrame[proc_id]] = 0;
    }
    pageTableFrame[proc_id] = -1;
    invalidate_tlb(proc_id);
}

void map_shared_memory(int proc_id) {
    if (proc_id < 0 || proc_id >= MAX_PROC) return;
    if (shared_memory_frame < 0) shared_memory_frame = getFreePage();

    int shared_page = (1024 + SHARED_MEMORY_ADDRESS) / PAGESIZE;
    int old_entry = get_page_entry(proc_id, shared_page);
    int old_frame = old_entry >> PTE_FRAME_SHIFT;
    if ((old_entry & PTE_VALID) && old_frame != shared_memory_frame &&
        old_frame > 0 && old_frame < NUM_PHYSICAL_PAGES) {
        freePages[old_frame] = 0;
    }
    set_page_entry(proc_id, shared_page, shared_memory_frame, PTE_READ | PTE_WRITE);
    invalidate_tlb(proc_id);
    log_system("[SHM] PID %d mapped shared frame %d at logical address %d\n",
               proc_id, shared_memory_frame, SHARED_MEMORY_ADDRESS);
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
        if (!(get_page_entry(proc_id, page_idx) & PTE_VALID)) {
            int frame = getFreePage();
            if (frame == -1) { fclose(fprog); fclose(fdata); return 0; }
            set_page_entry(proc_id, page_idx, frame, PTE_READ | PTE_EXECUTE);
        }

        int phys_addr = ((get_page_entry(proc_id, page_idx) >> PTE_FRAME_SHIFT) * PAGESIZE) + (inst_byte_count % PAGESIZE);
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
        if (!(get_page_entry(proc_id, logical_page_idx) & PTE_VALID)) {
            int frame = getFreePage();
            if (frame == -1) { fclose(fdata); return 0; }
            set_page_entry(proc_id, logical_page_idx, frame, PTE_READ | PTE_WRITE);
        }

        int phys_addr = ((get_page_entry(proc_id, logical_page_idx) >> PTE_FRAME_SHIFT) * PAGESIZE) + (data_byte_count % PAGESIZE);
        for (int b = 0; b < 4; b++) {
            memory[phys_addr + b] = (char)bytes[b];
        }

        // Check if FF FF FF FF sentinel is encountered
        if (bytes[0] == 0xFF && bytes[1] == 0xFF && bytes[2] == 0xFF && bytes[3] == 0xFF) {
            stop_after_current_page = 1; // Allow current page to complete, but stop at next page boundary
        }

        data_byte_count += 4;
    }
    printPageAllocation(proc_id);
    map_shared_memory(proc_id);

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
            int entry = get_page_entry(proc_id, page);
            int frame = entry >> PTE_FRAME_SHIFT;
            
            // Only write back pages that were allocated/loaded
            if (entry & PTE_VALID) {
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
    pcb.priority = 0;
    pcb.instructions_executed = 0;
    pcb.context_switches = 0;
    pcb.waiting_ticks = 0;

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
        save_context(pcb.proc_id, &pcb.context);

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
        if (!dequeue_highest_priority(&readyQueue, &process)) break;

        for (int waiting = 0; waiting < readyQueue.size; waiting++) {
            int index = (readyQueue.front + waiting) % QUEUE_CAPACITY;
            readyQueue.items[index].waiting_ticks++;
            if (readyQueue.items[index].priority < 10) readyQueue.items[index].priority++;
        }

        process.state = PROCESS_RUNNING;
        load_context(process.proc_id, &process.context);
        process.context_switches++;
        process_instructions(process.proc_id, BURST_TIME);
        save_context(process.proc_id, &process.context);
        process.instructions_executed = process_instruction_count[process.proc_id];

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
                } else if (strcmp(input_buffer, "stats") == 0) {
                    print_process_statistics();
                } else {
                    int pid, priority;
                    char command[PATH_LENGTH];
                    if (sscanf(input_buffer, "priority %d %d", &pid, &priority) == 2) {
                        set_process_priority(pid, priority);
                    } else if (sscanf(input_buffer, "memmap %d", &pid) == 1) {
                        print_memory_map(pid);
                    } else if (sscanf(input_buffer, "cache %d", &pid) == 1) {
                        print_memory_statistics(pid);
                    } else if (sscanf(input_buffer, "tlb %d", &pid) == 1) {
                        print_tlb_statistics(pid);
                    } else {
                        char prog_file[PATH_LENGTH], data_file[PATH_LENGTH];
                        if (sscanf(input_buffer, "%s %s", prog_file, data_file) == 2) {
                            loader(prog_file, data_file);
                        } else if (sscanf(input_buffer, "%s", command) == 1) {
                            log_system("Unknown command: %s\n", command);
                        }
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

void print_memory_map(int proc_id) {
    if (proc_id < 0 || proc_id >= MAX_PROC) {
        printf("Invalid process id %d\n", proc_id);
        return;
    }
    printPageAllocation(proc_id);
    printf("Shared buffer logical address: %d\n", SHARED_MEMORY_ADDRESS);
}

void set_process_priority(int pid, int priority) {
    for (int i = 0; i < readyQueue.size; i++) {
        int index = (readyQueue.front + i) % QUEUE_CAPACITY;
        if (readyQueue.items[index].pid == pid) {
            readyQueue.items[index].priority = priority;
            log_system("[SCHEDULER] PID %d priority set to %d\n", pid, priority);
            return;
        }
    }
    log_system("[SCHEDULER] PID %d not found in ready queue\n", pid);
}

void print_process_statistics(void) {
    for (int i = 0; i < readyQueue.size; i++) {
        int index = (readyQueue.front + i) % QUEUE_CAPACITY;
        PCB *process = &readyQueue.items[index];
        printf("PID %d priority=%d instructions=%lu switches=%lu waiting=%lu\n",
               process->pid, process->priority, process->instructions_executed,
               process->context_switches, process->waiting_ticks);
    }
}

void print_tlb_statistics(int proc_id) {
    if (proc_id < 0 || proc_id >= MAX_PROC) {
        printf("Invalid process id %d\n", proc_id);
        return;
    }
    printf("PID %d TLB hits=%lu misses=%lu\n", proc_id,
           tlb_hits[proc_id], tlb_misses[proc_id]);
}