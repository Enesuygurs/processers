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
    if (q->count < MAX_TASKS) {
        q->tasks[q->count++] = task;
    }
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
    
    /* Türkçe durum metinleri */
    const char* turkish_status = status;
    int extra_tab = 0;
    
    if (strcmp(status, "basladi") == 0) { turkish_status = "başladı"; extra_tab = 1; }
    else if (strcmp(status, "askida") == 0) { turkish_status = "askıda"; extra_tab = 1; }
    else if (strcmp(status, "yurutuluyor") == 0) { turkish_status = "yürütülüyor"; extra_tab = 0; }
    else if (strcmp(status, "sonlandi") == 0) { turkish_status = "sonlandı"; extra_tab = 1; }
    else if (strcmp(status, "zamanasimi") == 0) { turkish_status = "zamanaşımı"; extra_tab = 0; }
    
    if (extra_tab) {
        printf("%s%.4f sn\tproses %s\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
            task->color_code,
            (float)g_current_time,
            turkish_status,
            task->task_id,
            task->current_priority,
            task->remaining_time,
            COLOR_RESET);
    } else {
        printf("%s%.4f sn\tproses %s\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
            task->color_code,
            (float)g_current_time,
            turkish_status,
            task->task_id,
            task->current_priority,
            task->remaining_time,
            COLOR_RESET);
    }
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
        
        /* Timeout zamani: arrival_time + 20 saniye */
        int timeout_time = task->arrival_time + MAX_TASK_TIME;
        
        /* Gorev hic baslamadiysa: deadline aninda veya sonrasinda timeout */
        if (task->start_time == -1) {
            if (g_current_time >= timeout_time) {
                print_task_status(task, "zamanasimi");
                task->timeout_printed = 1;
                task_terminate(task, g_current_time);
                g_completed_tasks++;
            }
        }
        /* Gorev basladi ama askida kaldiysa: deadline gectikten sonra timeout */
        else {
            if (g_current_time > timeout_time) {
                int last_run_time = task->start_time + task->executed_time;
                /* Son calisma deadline'dan en az 3 saniye onceyse -> timeout */
                if (last_run_time < timeout_time - 2) {
                    print_task_status(task, "zamanasimi");
                    task->timeout_printed = 1;
                    task_terminate(task, g_current_time);
                    g_completed_tasks++;
                }
            }
        }
    }
}

void demote_priority(TaskInfo* task) {
    if (task == NULL) return;
    if (task->type == TASK_TYPE_REALTIME) return;
    
    /* Onceligi surekli dusur (sinir yok) */
    task->current_priority++;
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
            snprintf(task->task_name, sizeof(task->task_name), "Task%d", task_id + 1);
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
            
            task_id++;
        }
    }
    
    fclose(file);
    g_task_count = task_id;
    
    return task_id;
}

/*=============================================================================
 * ISTATISTIK FONKSIYONLARI
 *============================================================================*/

void print_statistics(void) {
    printf("\n%s", COLOR_RESET);
    printf("================================================================================\n");
    printf("                         SIMULASYON ISTATISTIKLERI                              \n");
    printf("================================================================================\n\n");
    
    int total_turnaround = 0;
    int total_waiting = 0;
    int total_response = 0;
    int total_burst = 0;
    int completed_count = 0;
    int timeout_count = 0;
    int rt_count = 0;
    int user_count = 0;
    
    printf("%-6s %-14s %-8s %-8s %-10s %-10s %-10s %-12s\n",
           "ID", "TIP", "VARIS", "BURST", "BASLAMA", "BITIS", "BEKLEME", "DURUM");
    printf("--------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < g_task_count; i++) {
        TaskInfo* task = &g_tasks[i];
        
        const char* type_str = get_task_type_string(task->type);
        const char* state_str = get_task_state_string(task->state);
        
        int turnaround = 0;
        int waiting = 0;
        int response = 0;
        
        if (task->state == TASK_STATE_TERMINATED && !task->timeout_printed) {
            turnaround = task->completion_time - task->arrival_time;
            waiting = turnaround - task->burst_time;
            if (waiting < 0) waiting = 0;
            if (task->start_time >= 0) {
                response = task->start_time - task->arrival_time;
            }
            
            total_turnaround += turnaround;
            total_waiting += waiting;
            total_response += response;
            total_burst += task->burst_time;
            completed_count++;
            
            if (task->type == TASK_TYPE_REALTIME) rt_count++;
            else user_count++;
        } else if (task->timeout_printed) {
            timeout_count++;
        }
        
        printf("%s%-6d %-14s %-8d %-8d %-10d %-10d %-10d %-12s%s\n",
               task->color_code,
               task->task_id,
               type_str,
               task->arrival_time,
               task->burst_time,
               task->start_time,
               task->completion_time,
               waiting,
               state_str,
               COLOR_RESET);
    }
    
    printf("--------------------------------------------------------------------------------\n\n");
    
    printf("OZET BILGILER:\n");
    printf("  Toplam Gorev Sayisi     : %d\n", g_task_count);
    printf("  Tamamlanan Gorevler     : %d\n", completed_count);
    printf("  Zaman Asimi Gorevler    : %d\n", timeout_count);
    printf("  Gercek Zamanli Gorevler : %d\n", rt_count);
    printf("  Kullanici Gorevleri     : %d\n", user_count);
    printf("  Context Switch Sayisi   : %d\n", g_context_switches);
    printf("  Toplam Simulasyon Suresi: %d saniye\n", g_current_time);
    
    if (completed_count > 0) {
        printf("\nPERFORMANS METRIKLERI:\n");
        printf("  Ortalama Donus Suresi   : %.2f saniye\n", (float)total_turnaround / completed_count);
        printf("  Ortalama Bekleme Suresi : %.2f saniye\n", (float)total_waiting / completed_count);
        printf("  Ortalama Yanit Suresi   : %.2f saniye\n", (float)total_response / completed_count);
        
        /* CPU Kullanim = Toplam Burst / Toplam Simulasyon Suresi * 100 */
        float cpu_usage = (g_current_time > 0) ? ((float)total_burst / g_current_time * 100) : 0;
        printf("  CPU Kullanim Orani      : %.1f%%\n", cpu_usage);
        printf("  Throughput              : %.2f gorev/saniye\n", (float)completed_count / g_current_time);
    }
    
    printf("\n================================================================================\n");
}
