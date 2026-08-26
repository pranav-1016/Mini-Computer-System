#include "queue.h"

void initQueue(CircularQueue *queue) {
    if (queue == 0) {
        return;
    }

    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
}

int isFull(const CircularQueue *queue) {
    return queue != 0 && queue->size == QUEUE_CAPACITY;
}

int isEmpty(const CircularQueue *queue) {
    return queue == 0 || queue->size == 0;
}

int enqueue(CircularQueue *queue, PCB process) {
    if (queue == 0 || isFull(queue)) {
        return 0;
    }

    queue->items[queue->rear] = process;
    queue->rear = (queue->rear + 1) % QUEUE_CAPACITY;
    queue->size++;
    return 1;
}

int dequeue(CircularQueue *queue, PCB *process) {
    if (isEmpty(queue) || process == 0) {
        return 0;
    }

    *process = queue->items[queue->front];
    queue->front = (queue->front + 1) % QUEUE_CAPACITY;
    queue->size--;
    return 1;
}

int peek(const CircularQueue *queue, PCB *process) {
    if (isEmpty(queue) || process == 0) {
        return 0;
    }

    *process = queue->items[queue->front];
    return 1;
}