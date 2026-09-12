#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#include "queue.h"
#include "processor.h"

extern CircularQueue readyQueue;
extern CircularQueue waitQueue;
extern int processor_busy[];
extern int shell_active;

// Helper check to see if any processor is currently executing
int has_active_processes(void) {
    if (!isEmpty(&readyQueue) || !isEmpty(&waitQueue)) {
        return 1;
    }
    for (int i = 0; i < NP; i++) {
        if (processor_busy[i]) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    // 1. Initialize terminal non-blocking IO and OS queues
    init_terminal();
    init_system_logs();

    log_system("[SYSTEM] Mini-Computer OS Started successfully.\n");

    // 2. Load initial process if passed via command line arguments
    if (argc >= 3) {
        log_system("Loading initial task: %s %s\n", argv[1], argv[2]);
        loader(argv[1], argv[2]);
    }

    // 3. System execution loop: runs while shell is open OR tasks are still executing
    while (shell_active || has_active_processes()) {
        scheduler();
    }

    // 4. Restore terminal state before exiting
    cleanup_system();
    printf("\nAll processes finished. Simulation terminated.\n");

    return 0;
}