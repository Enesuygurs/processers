/**
 * @file scheduler.h
 * @brief 4 Seviyeli Oncelikli Gorev Siralayici (Scheduler) Header Dosyasi
 * 
 * FreeRTOS kullanarak 4 seviyeli oncelik sistemi implementasyonu
 * 
 * @author Isletim Sistemleri Dersi Projesi
 * @date 2025
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/*=============================================================================
 * SABIT TANIMLAMALARI
 *============================================================================*/

#define MAX_TASKS               200
#define MAX_TASK_TIME           20      /* Maksimum gorev suresi (timeout) */
#define TIME_QUANTUM_MS         1000    /* 1 saniye = 1000 ms */
#define MAX_PRIORITY_LEVEL      20
#define COLOR_PALETTE_SIZE      25

/* Oncelik seviyeleri (dosyadan okunan 0-3) */
#define PRIORITY_REALTIME       0       /* Gercek zamanli */
#define PRIORITY_HIGH           1       /* Yuksek oncelikli kullanici */
#define PRIORITY_MEDIUM         2       /* Orta oncelikli kullanici */
#define PRIORITY_LOW            3       /* Dusuk oncelikli kullanici */

/* Terminal renk kodlari */
#define COLOR_RESET             "\033[0m"

/*=============================================================================
 * VERI YAPILARI
 *============================================================================*/

typedef enum {
    TASK_STATE_WAITING,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_SUSPENDED,
    TASK_STATE_TERMINATED
} TaskState;

typedef enum {
    TASK_TYPE_REALTIME,
    TASK_TYPE_USER
} TaskType;

typedef struct {
    int task_id;
    char task_name[32];
    int arrival_time;
    int original_priority;
    int current_priority;
    int burst_time;
    int remaining_time;
    int executed_time;
    TaskState state;
    TaskType type;
    int start_time;
    int completion_time;
    const char* color_code;
    int timeout_printed;
} TaskInfo;

typedef struct {
    TaskInfo* tasks[MAX_TASKS];
    int count;
} DynamicQueue;

/*=============================================================================
 * FONKSIYON PROTOTIPLERI
 *============================================================================*/

void init_queues(void);
void queue_add(int priority, TaskInfo* task);
TaskInfo* queue_remove(int priority);
int queue_is_empty(int priority);
int find_highest_priority_queue(void);

void check_arriving_tasks(void);
void check_timeouts(void);
void demote_priority(TaskInfo* task);

void print_task_status(TaskInfo* task, const char* status);

int load_tasks_from_file(const char* filename);

/*=============================================================================
 * GOREV YARDIMCI FONKSIYONLARI (tasks.c)
 *============================================================================*/

const char* get_task_state_string(TaskState state);
const char* get_task_type_string(TaskType type);
void print_task_info(TaskInfo* task);
void task_start(TaskInfo* task, int current_time);
void task_suspend(TaskInfo* task);
void task_resume(TaskInfo* task);
void task_terminate(TaskInfo* task, int current_time);
int task_execute(TaskInfo* task);
int task_is_ready(TaskInfo* task, int current_time);
int task_is_timeout(TaskInfo* task, int current_time);

#endif /* SCHEDULER_H */
