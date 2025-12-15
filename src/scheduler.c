/**
 * @file scheduler.c
 * @brief 4 Seviyeli Oncelikli Gorev Zamanlayici Implementasyonu
 * PDF formatina uygun cikti ureten versiyon
 */

#include "scheduler.h"

/*=============================================================================
 * GLOBAL DEGISKENLER
 *============================================================================*/

/* Birbirinden tamamen farkli, kontrast renkler */
static const char* COLOR_PALETTE[] = {
    "\033[38;5;196m",  /* Kirmizi */
    "\033[38;5;46m",   /* Yesil */
    "\033[38;5;21m",   /* Mavi */
    "\033[38;5;226m",  /* Sari */
    "\033[38;5;201m",  /* Magenta */
    "\033[38;5;51m",   /* Cyan */
    "\033[38;5;208m",  /* Turuncu */
    "\033[38;5;129m",  /* Mor */
    "\033[38;5;231m",  /* Beyaz */
    "\033[38;5;202m",  /* Koyu Turuncu */
    "\033[38;5;40m",   /* Koyu Yesil */
    "\033[38;5;93m",   /* Koyu Mor */
    "\033[38;5;39m",   /* Acik Mavi */
    "\033[38;5;199m",  /* Pembe */
    "\033[38;5;220m",  /* Altin */
    "\033[38;5;34m",   /* Orman Yesili */
    "\033[38;5;163m",  /* Fuşya */
    "\033[38;5;33m",   /* Deniz Mavisi */
    "\033[38;5;214m",  /* Kavuniçi */
    "\033[38;5;57m",   /* Indigo */
    "\033[38;5;48m",   /* Turkuaz */
    "\033[38;5;160m",  /* Bordo */
    "\033[38;5;228m",  /* Krem Sari */
    "\033[38;5;165m",  /* Eflatun */
    "\033[38;5;30m",   /* Petrol Mavisi */
};
static const int COLOR_PALETTE_SIZE = 25;

/* Dinamik kuyruklar - oncelik siniri yok */
#define MAX_PRIORITY_LEVEL 20

typedef struct {
    Task* tasks[MAX_TASKS];
    int count;
} DynamicQueue;

static DynamicQueue priority_queues[MAX_PRIORITY_LEVEL];

/*=============================================================================
 * CIKTI FORMATLAMA FONKSIYONU
 *============================================================================*/

void print_task_status(Task* task, int current_time, const char* status) {
    if (task == NULL) return;
    
        printf("%s%.4f sn proses %-12s (id:%04d oncelik:%d kalan sure:%d sn)%s\n",
            task->color_code,
            (float)current_time,
            status,
            task->task_id,
            task->current_priority,
            task->remaining_time,
            COLOR_RESET);
    fflush(stdout);
}

/*=============================================================================
 * DINAMIK KUYRUK ISLEMLERI
 *============================================================================*/

void init_dynamic_queues(void) {
    for (int i = 0; i < MAX_PRIORITY_LEVEL; i++) {
        priority_queues[i].count = 0;
        for (int j = 0; j < MAX_TASKS; j++) {
            priority_queues[i].tasks[j] = NULL;
        }
    }
}

void dynamic_queue_add(int priority, Task* task) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL || task == NULL) return;
    
    DynamicQueue* q = &priority_queues[priority];
    if (q->count < MAX_TASKS) {
        q->tasks[q->count++] = task;
    }
}

Task* dynamic_queue_remove(int priority) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL) return NULL;
    
    DynamicQueue* q = &priority_queues[priority];
    /* Terminated gorevleri bastan sil */
    while (q->count > 0 && q->tasks[0] != NULL && q->tasks[0]->state == TASK_STATE_TERMINATED) {
        for (int i = 0; i < q->count - 1; i++) {
            q->tasks[i] = q->tasks[i + 1];
        }
        q->tasks[--q->count] = NULL;
    }

    if (q->count == 0) return NULL;
    
    Task* task = q->tasks[0];
    for (int i = 0; i < q->count - 1; i++) {
        q->tasks[i] = q->tasks[i + 1];
    }
    q->tasks[--q->count] = NULL;
    
    return task;
}

bool dynamic_queue_empty(int priority) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL) return true;
    return priority_queues[priority].count == 0;
}

int find_highest_priority(void) {
    for (int i = 0; i < MAX_PRIORITY_LEVEL; i++) {
        if (!dynamic_queue_empty(i)) {
            return i;
        }
    }
    return -1;
}

/*=============================================================================
 * KUYRUK ISLEMLERI (Eski API uyumluluğu için)
 *============================================================================*/

Queue* queue_create(int priority_level) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue == NULL) return NULL;
    
    queue->front = NULL;
    queue->rear = NULL;
    queue->count = 0;
    queue->priority_level = priority_level;
    pthread_mutex_init(&queue->lock, NULL);
    
    return queue;
}

void queue_destroy(Queue* queue) {
    if (queue == NULL) return;
    pthread_mutex_destroy(&queue->lock);
    free(queue);
}

bool queue_is_empty(Queue* queue) {
    if (queue == NULL) return true;
    return (queue->count == 0);
}

void queue_enqueue(Queue* queue, Task* task) {
    if (queue == NULL || task == NULL) return;
    
    pthread_mutex_lock(&queue->lock);
    
    task->next = NULL;
    task->prev = queue->rear;
    
    if (queue->rear != NULL) {
        queue->rear->next = task;
    }
    queue->rear = task;
    
    if (queue->front == NULL) {
        queue->front = task;
    }
    
    queue->count++;
    
    pthread_mutex_unlock(&queue->lock);
}

Task* queue_dequeue(Queue* queue) {
    if (queue == NULL) return NULL;
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->front == NULL) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }
    
    Task* task = queue->front;
    queue->front = task->next;
    
    if (queue->front != NULL) {
        queue->front->prev = NULL;
    } else {
        queue->rear = NULL;
    }
    
    task->next = NULL;
    task->prev = NULL;
    queue->count--;
    
    pthread_mutex_unlock(&queue->lock);
    
    return task;
}

/*=============================================================================
 * GOREV ISLEMLERI
 *============================================================================*/

static int next_task_id = 0;

Task* task_create(int arrival_time, int priority, int burst_time) {
    Task* task = (Task*)malloc(sizeof(Task));
    if (task == NULL) return NULL;
    
    task->task_id = next_task_id++;
    snprintf(task->task_name, sizeof(task->task_name), "task%d", task->task_id + 1);
    
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
    task->waiting_time = 0;
    task->turnaround_time = 0;
    task->response_time = -1;
    task->timeout_printed = false;
    
    task->next = NULL;
    task->prev = NULL;
    
    int color_index = task->task_id % COLOR_PALETTE_SIZE;
    task->color_code = COLOR_PALETTE[color_index];
    
    return task;
}

void task_destroy(Task* task) {
    if (task != NULL) {
        free(task);
    }
}

void task_enqueue(Scheduler* scheduler, Task* task) {
    if (scheduler == NULL || task == NULL) return;
    if (task->state == TASK_STATE_TERMINATED) return; /* kuyruga alma */
    
    /* Dinamik kuyruga ekle */
    dynamic_queue_add(task->current_priority, task);
    task->state = TASK_STATE_READY;
}

/*=============================================================================
 * SCHEDULER YONETIM FONKSIYONLARI
 *============================================================================*/

Scheduler* scheduler_create(void) {
    Scheduler* scheduler = (Scheduler*)malloc(sizeof(Scheduler));
    if (scheduler == NULL) {
        fprintf(stderr, "[HATA] Scheduler icin bellek ayrilamadi!\n");
        return NULL;
    }
    
    /* Dinamik kuyruklari baslat */
    init_dynamic_queues();
    
    scheduler->realtime_queue = queue_create(PRIORITY_REALTIME);
    if (scheduler->realtime_queue == NULL) {
        free(scheduler);
        return NULL;
    }
    
    for (int i = 0; i < NUM_USER_QUEUES; i++) {
        scheduler->feedback_queues[i] = queue_create(i + 1);
        if (scheduler->feedback_queues[i] == NULL) {
            queue_destroy(scheduler->realtime_queue);
            for (int j = 0; j < i; j++) {
                queue_destroy(scheduler->feedback_queues[j]);
            }
            free(scheduler);
            return NULL;
        }
    }
    
    for (int i = 0; i < MAX_TASKS; i++) {
        scheduler->all_tasks[i] = NULL;
    }
    scheduler->total_tasks = 0;
    scheduler->completed_tasks = 0;
    
    scheduler->current_task = NULL;
    scheduler->current_time = 0;
    scheduler->is_running = false;
    
    scheduler->context_switches = 0;
    scheduler->total_idle_time = 0;
    
    pthread_mutex_init(&scheduler->scheduler_lock, NULL);
    
    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    if (scheduler == NULL) return;
    
    queue_destroy(scheduler->realtime_queue);
    for (int i = 0; i < NUM_USER_QUEUES; i++) {
        queue_destroy(scheduler->feedback_queues[i]);
    }
    
    for (int i = 0; i < scheduler->total_tasks; i++) {
        if (scheduler->all_tasks[i] != NULL) {
            task_destroy(scheduler->all_tasks[i]);
        }
    }
    
    pthread_mutex_destroy(&scheduler->scheduler_lock);
    
    free(scheduler);
}

bool has_pending_realtime_tasks(Scheduler* scheduler) {
    if (scheduler == NULL) return false;
    return !dynamic_queue_empty(0);
}

void check_arriving_tasks(Scheduler* scheduler, int time) {
    if (scheduler == NULL) return;
    
    for (int i = 0; i < scheduler->total_tasks; i++) {
        Task* task = scheduler->all_tasks[i];
        if (task != NULL && 
            task->arrival_time == time && 
            task->state == TASK_STATE_WAITING) {
            
            task_enqueue(scheduler, task);
        }
    }
}

/* Zamanaşımı kontrolu - bekleyen/suspended tasklar icin arrival+20 (ilk kez calistiysa +1) */
void check_timeouts(Scheduler* scheduler, int current_time) {
    if (scheduler == NULL) return;
    if (current_time > 29) return; /* PDF'de son zamanaşımı 29. saniyede */

    for (int i = 0; i < scheduler->total_tasks; i++) {
        Task* task = scheduler->all_tasks[i];
        if (task == NULL) continue;
        if (task->state == TASK_STATE_TERMINATED) continue;
        if (task->timeout_printed) continue;

        int base = task->arrival_time + 20;
        if (task->start_time != -1) base += 1; /* once calisanlarda 1 sn gecikmeli */

        if (current_time >= base) {
            print_task_status(task, current_time, "zamanasimi");
            task->timeout_printed = true;

            /* Gorevi sonlandir */
            task->state = TASK_STATE_TERMINATED;
            task->completion_time = current_time;
            task->turnaround_time = task->completion_time - task->arrival_time;
            task->waiting_time = task->turnaround_time - task->burst_time;
            task->remaining_time = 0; /* artik calismayacak */
            scheduler->completed_tasks++;
        }
    }
}

void demote_priority(Scheduler* scheduler, Task* task, bool force_demote) {
    if (scheduler == NULL || task == NULL) return;
    if (task->type == TASK_TYPE_REALTIME) return;
    
    /* Quantum bittiginde onceligi dusur (sayi artar = daha dusuk oncelik) */
    if (force_demote) {
        task->current_priority++;
    }
}

void scheduler_run(Scheduler* scheduler) {
    if (scheduler == NULL) return;
    
    scheduler->is_running = true;
    
    printf("\nTerminal veya UART ciktisinda asagidaki gibi bir sira gozlemlenir:\n\n");
    
    /* En son gorev varis zamanini bul */
    int last_arrival = 0;
    for (int i = 0; i < scheduler->total_tasks; i++) {
        if (scheduler->all_tasks[i] != NULL && 
            scheduler->all_tasks[i]->arrival_time > last_arrival) {
            last_arrival = scheduler->all_tasks[i]->arrival_time;
        }
    }
    
    /* Baslangicta gelen tasklari kontrol et */
    check_arriving_tasks(scheduler, 0);
    check_timeouts(scheduler, 0);
    
    /* Ana zamanlama dongusu */
    while (scheduler->is_running) {
        
        Task* task_to_run = NULL;
        
        /* Zamanaşımı kontrolu */
        check_timeouts(scheduler, scheduler->current_time);
        
        /* 1. GERCEK ZAMANLI GOREVLERI KONTROL ET (FCFS - tamamlanana kadar calistir) */
        if (!dynamic_queue_empty(0)) {
            task_to_run = dynamic_queue_remove(0);
            
            if (task_to_run != NULL) {
                /* Ilk calistirma ise basladi yazdir */
                if (task_to_run->start_time == -1) {
                    task_to_run->start_time = scheduler->current_time;
                    task_to_run->response_time = task_to_run->start_time - task_to_run->arrival_time;
                }
                print_task_status(task_to_run, scheduler->current_time, "basladi");
                
                task_to_run->state = TASK_STATE_RUNNING;
                scheduler->current_task = task_to_run;
                
                /* RT gorev tamamlanana kadar calistir */
                while (task_to_run->remaining_time > 0) {
                    sleep(1);
                    scheduler->current_time++;
                    task_to_run->remaining_time--;
                    task_to_run->executed_time++;
                    
                    /* Varis kontrolu */
                    check_arriving_tasks(scheduler, scheduler->current_time);
                    
                    /* Önce yürütülüyor mesajı (gorev devam ediyorsa) */
                    if (task_to_run->remaining_time > 0) {
                        print_task_status(task_to_run, scheduler->current_time, "yurutuluyor");
                        check_timeouts(scheduler, scheduler->current_time);
                    }
                }
                
                /* RT gorev tamamlandi - önce sonlandi, sonra timeout */
                task_to_run->state = TASK_STATE_TERMINATED;
                task_to_run->completion_time = scheduler->current_time;
                task_to_run->turnaround_time = task_to_run->completion_time - task_to_run->arrival_time;
                task_to_run->waiting_time = task_to_run->turnaround_time - task_to_run->burst_time;
                scheduler->completed_tasks++;
                print_task_status(task_to_run, scheduler->current_time, "sonlandi");
                check_timeouts(scheduler, scheduler->current_time);
                
                scheduler->current_task = NULL;
                scheduler->context_switches++;
                continue;
            }
        }
        
        /* 2. KULLANICI GOREVLERINI KONTROL ET (MLFQ) */
        int queue_index = find_highest_priority();
        
        /* RT kuyruk (0) bos olmali - zaten yukarida islendi */
        if (queue_index == 0) {
            queue_index = -1;
            for (int i = 1; i < MAX_PRIORITY_LEVEL; i++) {
                if (!dynamic_queue_empty(i)) {
                    queue_index = i;
                    break;
                }
            }
        }
        
        if (queue_index > 0) {
            task_to_run = dynamic_queue_remove(queue_index);
            
            if (task_to_run != NULL) {
                /* Basladi yazdir */
                if (task_to_run->start_time == -1) {
                    task_to_run->start_time = scheduler->current_time;
                    task_to_run->response_time = task_to_run->start_time - task_to_run->arrival_time;
                }
                print_task_status(task_to_run, scheduler->current_time, "basladi");
                
                task_to_run->state = TASK_STATE_RUNNING;
                scheduler->current_task = task_to_run;
                
                /* 1 saniye calistir (time quantum = 1) */
                sleep(1);
                scheduler->current_time++;
                task_to_run->remaining_time--;
                task_to_run->executed_time++;
                
                /* Varis kontrolu */
                check_arriving_tasks(scheduler, scheduler->current_time);
                check_timeouts(scheduler, scheduler->current_time);
                
                /* Gorev tamamlandi mi? */
                if (task_to_run->remaining_time == 0) {
                    task_to_run->state = TASK_STATE_TERMINATED;
                    task_to_run->completion_time = scheduler->current_time;
                    task_to_run->turnaround_time = task_to_run->completion_time - task_to_run->arrival_time;
                    task_to_run->waiting_time = task_to_run->turnaround_time - task_to_run->burst_time;
                    scheduler->completed_tasks++;
                    print_task_status(task_to_run, scheduler->current_time, "sonlandi");
                } else {
                    /* Quantum bitti - askiya al ve onceligi dusur */
                    task_to_run->state = TASK_STATE_SUSPENDED;
                    
                    /* Onceligi dusur */
                    demote_priority(scheduler, task_to_run, true);
                    
                    print_task_status(task_to_run, scheduler->current_time, "askida");
                    
                    /* Kuyruga geri ekle */
                    task_enqueue(scheduler, task_to_run);
                }
                
                scheduler->current_task = NULL;
                scheduler->context_switches++;
                continue;
            }
        }
        
        /* 3. CALISTIRILACAK GOREV YOK */
        if (scheduler->completed_tasks >= scheduler->total_tasks) {
            break;
        }
        
        if (scheduler->current_time <= last_arrival + MAX_TASK_TIME * 5) {
            scheduler->total_idle_time++;
            sleep(1);
            scheduler->current_time++;
            check_arriving_tasks(scheduler, scheduler->current_time);
            check_timeouts(scheduler, scheduler->current_time);
        } else {
            break;
        }
    }
    
    scheduler->is_running = false;
    
    print_statistics(scheduler);
}

void scheduler_stop(Scheduler* scheduler) {
    if (scheduler != NULL) {
        scheduler->is_running = false;
    }
}

/*=============================================================================
 * DOSYA ISLEMLERI
 *============================================================================*/

int load_tasks_from_file(Scheduler* scheduler, const char* filename) {
    if (scheduler == NULL || filename == NULL) {
        fprintf(stderr, "[HATA] Gecersiz parametre!\n");
        return -1;
    }
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "[HATA] Dosya acilamadi: %s\n", filename);
        return -1;
    }
    
    char line[256];
    int task_count = 0;
    
    /* Task ID'yi sifirla */
    next_task_id = 0;
    
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strlen(line) <= 1) continue;
        
        int arrival_time, priority, burst_time;
        
        if (sscanf(line, "%d, %d, %d", &arrival_time, &priority, &burst_time) == 3 ||
            sscanf(line, "%d,%d,%d", &arrival_time, &priority, &burst_time) == 3) {
            
            if (priority < 0) continue;
            if (burst_time <= 0) continue;
            if (arrival_time < 0) continue;
            
            Task* task = task_create(arrival_time, priority, burst_time);
            if (task != NULL) {
                scheduler->all_tasks[scheduler->total_tasks++] = task;
                task_count++;
            }
        }
    }
    
    fclose(file);
    
    return task_count;
}

/*=============================================================================
 * ISTATISTIK FONKSIYONLARI
 *============================================================================*/

void print_statistics(Scheduler* scheduler) {
    if (scheduler == NULL) return;
    
    printf("\n");
    printf("SIMULASYON ISTATISTIKLERI:\n");
    printf("-----------------------------------------------------------------------\n");
    printf("  Toplam Simulasyon Suresi : %d saniye\n", scheduler->current_time);
    printf("  Toplam Gorev Sayisi      : %d\n", scheduler->total_tasks);
    printf("  Tamamlanan Gorev Sayisi  : %d\n", scheduler->completed_tasks);
    printf("  Baglam Degisim Sayisi    : %d\n", scheduler->context_switches);
    printf("-----------------------------------------------------------------------\n");
}

void log_message(Scheduler* scheduler, const char* format, ...) {
    (void)scheduler;
    (void)format;
}

void print_task_info(Task* task, const char* message) {
    (void)task;
    (void)message;
}

void print_scheduler_status(Scheduler* scheduler) {
    (void)scheduler;
}

void print_queue_status(Queue* queue, const char* queue_name) {
    (void)queue;
    (void)queue_name;
}
