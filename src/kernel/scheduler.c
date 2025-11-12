#include "scheduler.h"
#include "kheap.h"
#include <string.h>

#define TASK_STACK_SIZE 4096

static task_t* current_task = NULL;
static task_t* ready_queue = NULL;
static uint32_t next_task_id = 1;

void scheduler_init(void) {
    current_task = NULL;
    ready_queue = NULL;
}

void scheduler_add_task(void (*entry)(void)) {
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    uint32_t* stack = (uint32_t*)kmalloc(TASK_STACK_SIZE);
    
    new_task->id = next_task_id++;
    new_task->esp = (uint32_t)(stack + TASK_STACK_SIZE / sizeof(uint32_t));
    new_task->ebp = new_task->esp;
    new_task->eip = (uint32_t)entry;
    new_task->state = TASK_READY;
    new_task->next = NULL;
    
    if (ready_queue == NULL) {
        ready_queue = new_task;
        current_task = new_task;
    } else {
        task_t* last = ready_queue;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_task;
    }
}

void schedule(void) {
    if (current_task == NULL || current_task->next == NULL) {
        return;
    }
    
    current_task = current_task->next;
    
    if (current_task == NULL) {
        current_task = ready_queue;
    }
}
