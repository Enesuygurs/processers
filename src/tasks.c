// Gorev simulasyon fonksiyonlari

#include "scheduler.h"

// Gorev yardimci fonksiyonlari
const char* get_task_state_string(TaskState state) {
    switch (state) {
        case TASK_STATE_WAITING:    return "BEKLIYOR";
        case TASK_STATE_READY:      return "HAZIR";
        case TASK_STATE_RUNNING:    return "CALISIYOR";
        case TASK_STATE_SUSPENDED:  return "ASKIDA";
        case TASK_STATE_TERMINATED: return "SONLANDI";
        default:                    return "BILINMIYOR";
    }
}

const char* get_task_type_string(TaskType type) {
    switch (type) {
        case TASK_TYPE_REALTIME: return "GERCEK ZAMANLI";
        case TASK_TYPE_USER:     return "KULLANICI";
        default:                 return "BILINMIYOR";
    }
}

void print_task_info(TaskInfo* task) {
    if (task == NULL) return;
    
    printf("Gorev ID: %d\n", task->task_id);
    printf("  Varis Zamani: %d\n", task->arrival_time);
    printf("  Oncelik: %d\n", task->current_priority);
    printf("  Kalan Sure: %d sn\n", task->remaining_time);
    printf("  Durum: %s\n", get_task_state_string(task->state));
    printf("  Tip: %s\n", get_task_type_string(task->type));
}

void task_start(TaskInfo* task, int current_time) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_RUNNING;
    if (task->start_time == -1) {
        task->start_time = current_time;
    }
}

void task_suspend(TaskInfo* task) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_SUSPENDED;
}

void task_resume(TaskInfo* task) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_READY;
}

void task_terminate(TaskInfo* task, int current_time) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_TERMINATED;
    task->completion_time = current_time;
}

int task_execute(TaskInfo* task) {
    if (task == NULL) return -1;
    
    task->remaining_time--;
    task->executed_time++;
    
    return task->remaining_time;
}

int task_is_ready(TaskInfo* task, int current_time) {
    if (task == NULL) return 0;
    
    return (task->arrival_time <= current_time && 
            task->state != TASK_STATE_TERMINATED &&
            task->remaining_time > 0);
}

int task_is_timeout(TaskInfo* task, int current_time) {
    if (task == NULL) return 0;
    
    int timeout_time = task->arrival_time + MAX_TASK_TIME;
    // Deadline gecildikten sonra timeout
    return (current_time > timeout_time && task->state != TASK_STATE_TERMINATED);
}
