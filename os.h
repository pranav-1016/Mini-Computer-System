#ifndef OS_H
#define OS_H

#include "queue.h"
// #include<filesystem.h>
// #define BURST_TIME = 10
// #define SLEEP_TIME = 20
#define PATH_LENGTH 256

// data elements in the OS 
/*
    - list of processes in ready and waiting queue
    - current active processes
    - processes to process id mappings
*/

extern CircularQueue readyQueue;
extern CircularQueue waitQueue;


void init_terminal();
void reset_keyboard();
// loads the program from the program folder, and corresponding data.byte file from the data folder
void loader(const char *program_file, const char *data_file);

void scheduler(void);

void shell();

#endif