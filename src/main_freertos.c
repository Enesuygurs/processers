/**
 * @file main_freertos.c
 * @brief FreeRTOS Gorev Siralayici Simulasyonu - Ana Program
 * 
 * 4 Seviyeli Oncelikli Gorevlendirici (Scheduler) Simulasyonu
 * - Seviye 0: Gercek Zamanli (Real-Time) - FCFS algoritmasi
 * - Seviye 1-3: Kullanici Gorevleri - Multi-Level Feedback Queue (MLFQ)
 * 
 * @author Isletim Sistemleri Dersi Projesi
 * @date 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

/* FreeRTOS header dosyalari */
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

/* Oncelik seviyeleri (dosyadan okunan 0-3) */
#define PRIORITY_REALTIME       0       /* Gercek zamanli */
#define PRIORITY_HIGH           1       /* Yuksek oncelikli kullanici */
#define PRIORITY_MEDIUM         2       /* Orta oncelikli kullanici */
#define PRIORITY_LOW            3       /* Dusuk oncelikli kullanici */

/* Terminal renk kodlari */
#define COLOR_RESET             "\033[0m"
static const char* COLOR_PALETTE[] = {
    "\033[38;5;196m", "\033[38;5;46m",  "\033[38;5;21m",  "\033[38;5;226m",
    "\033[38;5;201m", "\033[38;5;51m",  "\033[38;5;208m", "\033[38;5;129m",
    "\033[38;5;231m", "\033[38;5;202m", "\033[38;5;40m",  "\033[38;5;93m",
    "\033[38;5;39m",  "\033[38;5;199m", "\033[38;5;220m", "\033[38;5;34m",
    "\033[38;5;163m", "\033[38;5;33m",  "\033[38;5;214m", "\033[38;5;57m",
    "\033[38;5;48m",  "\033[38;5;160m", "\033[38;5;228m", "\033[38;5;165m",
    "\033[38;5;30m"
};
#define COLOR_PALETTE_SIZE 25

/*=============================================================================
 * VERI YAPILARI
 *============================================================================*/

/* Gorev Durumu */
typedef enum {
    TASK_STATE_WAITING,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_SUSPENDED,
    TASK_STATE_TERMINATED
} TaskState;

/* Gorev Tipi */
typedef enum {
    TASK_TYPE_REALTIME,
    TASK_TYPE_USER
} TaskType;

/* Gorev Bilgi Yapisi */
typedef struct {
    int task_id;
    char task_name[32];
    int arrival_time;
    int original_priority;      /* Dosyadan okunan (0-3) */
    int current_priority;       /* Mevcut oncelik */
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

/* Dinamik Kuyruk Yapisi */
typedef struct {
    TaskInfo* tasks[MAX_TASKS];
    int count;
} DynamicQueue;

/*=============================================================================
 * GLOBAL DEGISKENLER
 *============================================================================*/

static TaskInfo g_tasks[MAX_TASKS];
static int g_task_count = 0;
static int g_completed_tasks = 0;
static int g_current_time = 0;
static int g_context_switches = 0;
static volatile int g_simulation_running = 1;

/* Dinamik kuyruklar - oncelik 0=RT, 1-3=Kullanici */
#define MAX_PRIORITY_LEVEL 20
static DynamicQueue g_priority_queues[MAX_PRIORITY_LEVEL];

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

/* Gorev durum mesaji yazdir */
void print_task_status(TaskInfo* task, const char* status) {
    if (task == NULL) return;
    printf("%s%.4f sn proses %-12s (id:%04d oncelik:%d kalan sure:%d sn)%s\n",
        task->color_code,
        (float)g_current_time,
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

/* Varan gorevleri kontrol et ve kuyruga ekle */
void check_arriving_tasks(void) {
    for (int i = 0; i < g_task_count; i++) {
        TaskInfo* task = &g_tasks[i];
        
        if (task->arrival_time == g_current_time && task->state == TASK_STATE_WAITING) {
            task->state = TASK_STATE_READY;
            queue_add(task->current_priority, task);
        }
    }
}

/* Zaman asimi kontrolu */
void check_timeouts(void) {
    for (int i = 0; i < g_task_count; i++) {
        TaskInfo* task = &g_tasks[i];
        
        if (task->state == TASK_STATE_TERMINATED) continue;
        if (task->timeout_printed) continue;
        if (task->state == TASK_STATE_WAITING) continue;
        
        int timeout_time = task->arrival_time + MAX_TASK_TIME;
        if (g_current_time >= timeout_time) {
            print_task_status(task, "zamanasimi");
            task->timeout_printed = 1;
            task->state = TASK_STATE_TERMINATED;
            task->completion_time = g_current_time;
            g_completed_tasks++;
        }
    }
}

/* Oncelik dusurme (MLFQ) */
void demote_priority(TaskInfo* task) {
    if (task == NULL) return;
    if (task->type == TASK_TYPE_REALTIME) return;
    
    /* Sadece 3'e kadar dusur (PRIORITY_LOW) */
    if (task->current_priority < PRIORITY_LOW) {
        task->current_priority++;
    }
}

/*=============================================================================
 * ANA SCHEDULER GOREVI
 *============================================================================*/

void vSchedulerTask(void* pvParameters) {
    (void)pvParameters;
    
    printf("\nTerminal veya UART ciktisinda asagidaki gibi bir sira gozlemlenir:\n\n");
    
    /* En son gorev varis zamanini bul */
    int last_arrival = 0;
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].arrival_time > last_arrival) {
            last_arrival = g_tasks[i].arrival_time;
        }
    }
    
    /* Baslangicta gelen gorevleri kontrol et */
    check_arriving_tasks();
    
    /* Ana zamanlama dongusu */
    while (g_simulation_running) {
        TaskInfo* task_to_run = NULL;
        
        /* Zamanaşımı kontrolu */
        check_timeouts();
        
        /* 1. GERCEK ZAMANLI GOREVLERI KONTROL ET (FCFS - tamamlanana kadar calistir) */
        if (!queue_is_empty(PRIORITY_REALTIME)) {
            task_to_run = queue_remove(PRIORITY_REALTIME);
            
            if (task_to_run != NULL && task_to_run->state != TASK_STATE_TERMINATED) {
                /* Ilk calistirma ise */
                if (task_to_run->start_time == -1) {
                    task_to_run->start_time = g_current_time;
                }
                print_task_status(task_to_run, "basladi");
                
                task_to_run->state = TASK_STATE_RUNNING;
                
                /* RT gorev tamamlanana kadar calistir */
                while (task_to_run->remaining_time > 0) {
                    vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM_MS));
                    g_current_time++;
                    task_to_run->remaining_time--;
                    task_to_run->executed_time++;
                    
                    /* Varis kontrolu */
                    check_arriving_tasks();
                    
                    /* Gorev devam ediyorsa */
                    if (task_to_run->remaining_time > 0) {
                        print_task_status(task_to_run, "yurutuluyor");
                        check_timeouts();
                    }
                }
                
                /* RT gorev tamamlandi */
                task_to_run->state = TASK_STATE_TERMINATED;
                task_to_run->completion_time = g_current_time;
                g_completed_tasks++;
                print_task_status(task_to_run, "sonlandi");
                check_timeouts();
                
                g_context_switches++;
                continue;
            }
        }
        
        /* 2. KULLANICI GOREVLERINI KONTROL ET (MLFQ) */
        int queue_index = find_highest_priority_queue();
        
        /* RT kuyruk (0) zaten yukarida islendi */
        if (queue_index == PRIORITY_REALTIME) {
            queue_index = -1;
            for (int i = PRIORITY_HIGH; i < MAX_PRIORITY_LEVEL; i++) {
                if (!queue_is_empty(i)) {
                    queue_index = i;
                    break;
                }
            }
        }
        
        if (queue_index > 0) {
            task_to_run = queue_remove(queue_index);
            
            if (task_to_run != NULL && task_to_run->state != TASK_STATE_TERMINATED) {
                /* Ilk calistirma ise */
                if (task_to_run->start_time == -1) {
                    task_to_run->start_time = g_current_time;
                    print_task_status(task_to_run, "basladi");
                } else {
                    /* Devam eden gorev */
                    print_task_status(task_to_run, "devam");
                }
                
                task_to_run->state = TASK_STATE_RUNNING;
                
                /* 1 saniye calistir (time quantum = 1) */
                vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM_MS));
                g_current_time++;
                task_to_run->remaining_time--;
                task_to_run->executed_time++;
                
                /* Varis kontrolu */
                check_arriving_tasks();
                check_timeouts();
                
                /* Gorev tamamlandi mi? */
                if (task_to_run->remaining_time == 0) {
                    task_to_run->state = TASK_STATE_TERMINATED;
                    task_to_run->completion_time = g_current_time;
                    g_completed_tasks++;
                    print_task_status(task_to_run, "sonlandi");
                } else {
                    /* Quantum bitti - askiya al */
                    task_to_run->state = TASK_STATE_SUSPENDED;
                    
                    /* Onceligi dusur */
                    demote_priority(task_to_run);
                    
                    print_task_status(task_to_run, "askida");
                    
                    /* Kuyruga geri ekle */
                    task_to_run->state = TASK_STATE_READY;
                    queue_add(task_to_run->current_priority, task_to_run);
                }
                
                g_context_switches++;
                continue;
            }
        }
        
        /* 3. CALISTIRILACAK GOREV YOK */
        if (g_completed_tasks >= g_task_count) {
            break;
        }
        
        /* Bekleme - yeni gorevlerin gelmesini bekle */
        if (g_current_time <= last_arrival + MAX_TASK_TIME + 10) {
            vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM_MS));
            g_current_time++;
            check_arriving_tasks();
            check_timeouts();
        } else {
            break;
        }
    }
    
    g_simulation_running = 0;
    
    /* Istatistikleri yazdir */
    printf("\n");
    printf("SIMULASYON ISTATISTIKLERI:\n");
    printf("-----------------------------------------------------------------------\n");
    printf("  Toplam Simulasyon Suresi : %d saniye\n", g_current_time);
    printf("  Toplam Gorev Sayisi      : %d\n", g_task_count);
    printf("  Tamamlanan Gorev Sayisi  : %d\n", g_completed_tasks);
    printf("  Baglam Degisim Sayisi    : %d\n", g_context_switches);
    printf("-----------------------------------------------------------------------\n");
    
    /* Simulasyonu sonlandir */
    vTaskEndScheduler();
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
    
    printf("[BILGI] %d gorev yuklendi.\n", g_task_count);
    return task_id;
}

/*=============================================================================
 * FREERTOS HOOK FONKSIYONLARI
 *============================================================================*/

void vApplicationIdleHook(void) {
    /* Bos */
}

void vApplicationTickHook(void) {
    /* Bos */
}

/* Static allocation icin gerekli */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = xIdleStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configMINIMAL_STACK_SIZE * 2];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulTimerTaskStackSize) {
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
    *ppxTimerTaskStackBuffer = xTimerStack;
    *pulTimerTaskStackSize = configMINIMAL_STACK_SIZE * 2;
}

/*=============================================================================
 * ANA FONKSIYON
 *============================================================================*/

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Kullanim: %s <giris_dosyasi>\n", argv[0]);
        printf("Ornek: %s giris.txt\n", argv[0]);
        return 1;
    }
    
    printf("==========================================================\n");
    printf("  FreeRTOS Gorev Siralayici Simulasyonu\n");
    printf("  4 Seviyeli Oncelikli Gorevlendirici (MLFQ + FCFS)\n");
    printf("==========================================================\n\n");
    
    /* Kuyruklari baslat */
    init_queues();
    
    /* Gorevleri dosyadan yukle */
    if (load_tasks_from_file(argv[1]) <= 0) {
        printf("[HATA] Gorev yuklenemedi!\n");
        return 1;
    }
    
    /* Scheduler gorevini olustur */
    xTaskCreate(
        vSchedulerTask,
        "Scheduler",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL
    );
    
    /* FreeRTOS scheduler'i baslat */
    printf("[BILGI] FreeRTOS Scheduler baslatiliyor...\n");
    vTaskStartScheduler();
    
    /* Scheduler sonlandi - normal cikis */
    return 0;
}
