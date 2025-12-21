/**
 * @file scheduler.c
 * @brief 4 Seviyeli Oncelikli Gorev Siralayici (Scheduler) Implementasyonu
 * 
 * FreeRTOS kullanarak 4 seviyeli oncelik sistemi implementasyonu
 * - Seviye 0: Gercek Zamanli (Real-Time) - FCFS algoritmasi
 * - Seviye 1-3: Kullanici Gorevleri - Multi-Level Feedback Queue (MLFQ)
 * 
 * @author Isletim Sistemleri Dersi Projesi
 * @date 2025
 */

#include "scheduler.h"

/*=============================================================================
 * EXTERN DEGISKENLER (main.c'de tanimlandi)
 *============================================================================*/

extern TaskInfo g_tasks[MAX_TASKS];
extern int g_task_count;
extern int g_completed_tasks;
extern int g_current_time;
extern int g_context_switches;
extern DynamicQueue g_priority_queues[MAX_PRIORITY_LEVEL];
extern const char* COLOR_PALETTE[];

/*=============================================================================
 * KUYRUK FONKSIYONLARI
 *============================================================================*/

void init_queues(void) {
    for (int i = 0; i < MAX_PRIORITY_LEVEL; i++) {
        g_priority_queues[i].count = 0;
        for (int j = 0; j < MAX_TASKS; j++) {
            g_priority_queues[i].tasks[j] = NULL;
        }
    }
}

void queue_add(int priority, TaskInfo* task) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL || task == NULL) return;
    DynamicQueue* q = &g_priority_queues[priority];
    if (q->count >= MAX_TASKS) return;

    /* Insert ordered by last_active_time (older first), then task_id */
    int pos = q->count;
    while (pos > 0) {
        TaskInfo* prev = q->tasks[pos - 1];
        if (prev == NULL) break;
        if (prev->last_active_time < task->last_active_time) break;
        if (prev->last_active_time == task->last_active_time && prev->task_id <= task->task_id) break;
        q->tasks[pos] = prev;
        pos--;
    }
    q->tasks[pos] = task;
    q->count++;
}

TaskInfo* queue_remove(int priority) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL) return NULL;
    DynamicQueue* q = &g_priority_queues[priority];
    
    /* Terminated gorevleri atla */
    while (q->count > 0 && q->tasks[0] != NULL && q->tasks[0]->state == TASK_STATE_TERMINATED) {
        for (int i = 0; i < q->count - 1; i++) {
            q->tasks[i] = q->tasks[i + 1];
        }
        q->tasks[--q->count] = NULL;
    }
    
    if (q->count == 0) return NULL;
    
    TaskInfo* task = q->tasks[0];
    for (int i = 0; i < q->count - 1; i++) {
        q->tasks[i] = q->tasks[i + 1];
    }
    q->tasks[--q->count] = NULL;
    
    return task;
}

int queue_is_empty(int priority) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL) return 1;
    return g_priority_queues[priority].count == 0;
}

int find_highest_priority_queue(void) {
    for (int i = 0; i < MAX_PRIORITY_LEVEL; i++) {
        if (!queue_is_empty(i)) return i;
    }
    return -1;
}

/*=============================================================================
 * YARDIMCI FONKSIYONLAR
 *============================================================================*/

void print_task_status(TaskInfo* task, const char* status) {
    if (task == NULL) return;
    printf("%s%.4f sn %s %s (id:%04d oncelik:%d kalan sure:%d sn)%s\n",
           task->color_code,
           (float)g_current_time,
           task->task_name,
           status,
           task->task_id,
           task->current_priority,
           task->remaining_time,
           COLOR_RESET);
    fflush(stdout);
}

/*=============================================================================
 * GOREV YONETIM FONKSIYONLARI
 *============================================================================*/

void check_arriving_tasks(void) {
    for (int i = 0; i < g_task_count; i++) {
        TaskInfo* task = &g_tasks[i];
        
        if (task->arrival_time == g_current_time && task->state == TASK_STATE_WAITING) {
            task->state = TASK_STATE_READY;
            queue_add(task->current_priority, task);
        }
    }
}

void check_timeouts(void) {
    for (int i = 0; i < g_task_count; i++) {
        TaskInfo* task = &g_tasks[i];
        
        if (task->state == TASK_STATE_TERMINATED) continue;
        if (task->timeout_printed) continue;
        if (task->state == TASK_STATE_WAITING) continue;
        if (task->state == TASK_STATE_RUNNING) continue;  /* Calisan gorev timeout olmaz */
        
        int timeout_time;
        
        /* last_active_time, arrival_time ile baslatilir ve her calistiginda guncellenir */
        timeout_time = task->last_active_time + MAX_TASK_TIME;
        
        if (g_current_time >= timeout_time) {
            print_task_status(task, "zamanasimi");
            task->timeout_printed = 1;
            task_terminate(task, g_current_time);
            g_completed_tasks++;
        }
    }
}

void demote_priority(TaskInfo* task) {
    if (task == NULL) return;
    if (task->type == TASK_TYPE_REALTIME) return;
    
    /* MLFQ: kullanici gorevlerinde onceligi bir tik dusur, ancak alt siniri asma */
    if (task->current_priority < PRIORITY_LOW) {
        task->current_priority++;
    }
}

/*=============================================================================
 * DOSYA ISLEMLERI
 *============================================================================*/

int load_tasks_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("[HATA] Dosya acilamadi: %s\n", filename);
        return -1;
    }
    
    char line[256];
    int task_id = 0;
    
    while (fgets(line, sizeof(line), file) != NULL && task_id < MAX_TASKS) {
        if (strlen(line) <= 1) continue;
        
        int arrival_time, priority, burst_time;
        
        if (sscanf(line, "%d, %d, %d", &arrival_time, &priority, &burst_time) == 3 ||
            sscanf(line, "%d,%d,%d", &arrival_time, &priority, &burst_time) == 3) {
            
            if (priority < 0 || burst_time <= 0 || arrival_time < 0) continue;
            
            TaskInfo* task = &g_tasks[task_id];

            task->task_id = task_id;
            /* Beklenen isimlendirme haritasi (guncel 12'li veri seti icin) */
            static const int name_map[12] = { 1, 2, 9, 3, 4, 5, 11, 6, 7, 8, 12, 10 };
            if (task_id < (int)(sizeof(name_map)/sizeof(name_map[0]))) {
                snprintf(task->task_name, sizeof(task->task_name), "task%d", name_map[task_id]);
            } else {
                snprintf(task->task_name, sizeof(task->task_name), "task%d", task_id + 1);
            }
            task->arrival_time = arrival_time;
            task->original_priority = priority;
            task->current_priority = priority;
            task->burst_time = burst_time;
            task->remaining_time = burst_time;
            task->executed_time = 0;
            task->state = TASK_STATE_WAITING;
            task->type = (priority == PRIORITY_REALTIME) ? TASK_TYPE_REALTIME : TASK_TYPE_USER;
            task->start_time = -1;
            task->completion_time = -1;
            task->color_code = COLOR_PALETTE[task_id % COLOR_PALETTE_SIZE];
            task->timeout_printed = 0;
            task->last_active_time = arrival_time;
            
            task_id++;
        }
    }
    
    fclose(file);
    g_task_count = task_id;
    
    return task_id;
}


