/**
 * @file tasks.c
 * @brief FreeRTOS Gorev Simulasyon Fonksiyonlari
 */

#include "scheduler.h"
#include <sys/time.h>

/*=============================================================================
 * ZAMANLAMA YARDIMCI FONKSIYONLARI
 *============================================================================*/

long get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void sleep_ms(int ms) {
    usleep(ms * 1000);
}

/*=============================================================================
 * GANTT SEMASI FONKSIYONLARI
 *============================================================================*/

typedef struct GanttEntry {
    int task_id;
    int start_time;
    int end_time;
    int priority;
    const char* color;
    char task_name[32];
} GanttEntry;

static GanttEntry gantt_chart[1000];
static int gantt_count = 0;

void gantt_add_entry(Task* task, int start, int end) {
    if (task == NULL || gantt_count >= 1000) return;
    
    gantt_chart[gantt_count].task_id = task->task_id;
    gantt_chart[gantt_count].start_time = start;
    gantt_chart[gantt_count].end_time = end;
    gantt_chart[gantt_count].priority = task->current_priority;
    gantt_chart[gantt_count].color = task->color_code;
    strncpy(gantt_chart[gantt_count].task_name, task->task_name, 31);
    gantt_chart[gantt_count].task_name[31] = '\0';
    
    gantt_count++;
}

void print_gantt_chart(Scheduler* scheduler) {
    (void)scheduler;
}

void gantt_reset(void) {
    gantt_count = 0;
}

/*=============================================================================
 * GOREV GORSELLISTIRME FONKSIYONLARI (bos - scheduler.c'de tanimli)
 *============================================================================*/

void print_task_start_message(Task* task, int current_time) {
    (void)task;
    (void)current_time;
}

void print_task_running_message(Task* task, int current_time, int elapsed) {
    (void)task;
    (void)current_time;
    (void)elapsed;
}

void print_task_suspend_message(Task* task, int current_time) {
    (void)task;
    (void)current_time;
}

void print_task_resume_message(Task* task, int current_time) {
    (void)task;
    (void)current_time;
}

void print_task_complete_message(Task* task, int current_time) {
    (void)task;
    (void)current_time;
}

int simulate_task_execution(Task* task, Scheduler* scheduler) {
    (void)task;
    (void)scheduler;
    return 0;
}

void visualize_all_queues(Scheduler* scheduler) {
    (void)scheduler;
}

void print_system_status(Scheduler* scheduler) {
    (void)scheduler;
}

void print_startup_banner(void) {
    /* Bos - banner yok */
}

void print_algorithm_info(void) {
    /* Bos - algoritma bilgisi yok */
}
