#ifndef OS_H
#define OS_H

#include <stdio.h>
#include "queue.h"
#include "memory.h"

#define BURST_TIME 2
#define SLEEP_TIME 200000
#define PATH_LENGTH 256
#define MAX_PROC 4

#define NUM_PHYSICAL_PAGES (MEMSIZE / PAGESIZE)
#define NUM_LOGICAL_PAGES ((1024 + 4096) / PAGESIZE)
#define PTE_VALID 0x01
#define PTE_READ 0x02
#define PTE_WRITE 0x04
#define PTE_EXECUTE 0x08
#define PTE_FRAME_SHIFT 8
#define PTE_FRAME_MASK 0xFFFFFF00
#define TLB_ENTRIES 8
#define SHARED_MEMORY_ADDRESS (DATA_MEM_SIZE - PAGESIZE)

// Global Queue Declarations
extern CircularQueue readyQueue;
extern CircularQueue waitQueue;

// System Log File Pointers
extern FILE *fd_log;
extern FILE *fd_system_log;

// Page Management & MMU Translation
int getFreePage(void);
int getPhysicalAddress(int proc_id, int access_type, int address);
int get_page_entry(int proc_id, int logical_page);
int set_page_entry(int proc_id, int logical_page, int frame, int permissions);
void map_shared_memory(int proc_id);
void freeProcessPages(int proc_id);

// System Initialization & Logging
void init_system_logs(void);
void log_system(const char *format, ...);
void init_terminal(void);
void reset_keyboard(void);
void cleanup_system(void);

// Process Lifecycle & Scheduling
void loader(const char *program_file, const char *data_file);
void initialise(int proc_id, const char *program_file, const char *data_file);
void finalize(int proc_id, const char *output_data_file);
void set_process_priority(int pid, int priority);
void print_process_statistics(void);
void print_memory_map(int proc_id);
void print_tlb_statistics(int proc_id);
void scheduler(void);
void shell(void);

#endif