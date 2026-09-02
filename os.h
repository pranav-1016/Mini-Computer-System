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

// Global Queue Declarations
extern CircularQueue readyQueue;
extern CircularQueue waitQueue;

// System Log File Pointers
extern FILE *fd_log;
extern FILE *fd_system_log;

// Page Management & MMU Translation
int getFreePage(void);
int getPhysicalAddress(int proc_id, int isFetch, int address);
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
void scheduler(void);
void shell(void);

#endif