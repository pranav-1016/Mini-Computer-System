#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_CAPACITY 32
#define PATH_LENGTH 256

typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED
} ProcessState;

typedef struct PCB {
    int pid;
    int proc_id;
    char program_file[PATH_LENGTH];
    char data_file[PATH_LENGTH];
    ProcessState state;
} PCB;

typedef struct CircularQueue {
    PCB items[QUEUE_CAPACITY];
    int front;
    int rear;
    int size;
} CircularQueue;

void initQueue(CircularQueue *queue);
int isFull(const CircularQueue *queue);
int isEmpty(const CircularQueue *queue);
int enqueue(CircularQueue *queue, PCB process);
int dequeue(CircularQueue *queue, PCB *process);
int peek(const CircularQueue *queue, PCB *process);




#endif