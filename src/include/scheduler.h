#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED
} task_state_t;

typedef struct task {
    uint32_t id;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    task_state_t state;
    struct task* next;
} task_t;

void scheduler_init(void);
void scheduler_add_task(void (*entry)(void));
void schedule(void);

#endif /* SCHEDULER_H */
