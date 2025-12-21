// FreeRTOS Gorev Siralayici - Ana Program
// RT (FCFS) + Kullanici Gorevleri (MLFQ)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

// FreeRTOS header dosyalari
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

// Proje header dosyalari
#include "scheduler.h"

// Renk paleti tanimlamasi
const char* COLOR_PALETTE[] = {
    "\033[38;5;196m", "\033[38;5;46m",  "\033[38;5;21m",  "\033[38;5;226m",
    "\033[38;5;201m", "\033[38;5;51m",  "\033[38;5;208m", "\033[38;5;129m",
    "\033[38;5;231m", "\033[38;5;202m", "\033[38;5;40m",  "\033[38;5;93m",
    "\033[38;5;39m",  "\033[38;5;199m", "\033[38;5;220m", "\033[38;5;34m",
    "\033[38;5;163m", "\033[38;5;33m",  "\033[38;5;214m", "\033[38;5;57m",
    "\033[38;5;48m",  "\033[38;5;160m", "\033[38;5;228m", "\033[38;5;165m",
    "\033[38;5;30m"
};
#define COLOR_PALETTE_SIZE 25

// Global degiskenler
TaskInfo g_tasks[MAX_TASKS];
int g_task_count = 0;
int g_completed_tasks = 0;
int g_current_time = 0;
int g_context_switches = 0;
static volatile int g_simulation_running = 1;

// Dinamik kuyruklar: 0=RT, 1-3=Kullanici
DynamicQueue g_priority_queues[MAX_PRIORITY_LEVEL];

// Ana scheduler gorevi
void vSchedulerTask(void* pvParameters) {
    (void)pvParameters;
    
    // En son gorev varis zamanini bul
    int last_arrival = 0;
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].arrival_time > last_arrival) {
            last_arrival = g_tasks[i].arrival_time;
        }
    }
    
    // Baslangicta gelen gorevleri kontrol et
    check_arriving_tasks();
    
    // Ana zamanlama dongusu
    while (g_simulation_running) {
        TaskInfo* task_to_run = NULL;
        
        // Zamantasimi kontrolu
        check_timeouts();
        
        // 1. RT gorevleri kontrol et (FCFS)
        if (!queue_is_empty(PRIORITY_REALTIME)) {
            task_to_run = queue_remove(PRIORITY_REALTIME);
            
            if (task_to_run != NULL && task_to_run->state != TASK_STATE_TERMINATED) {
                // Gorevi baslat
                task_start(task_to_run, g_current_time);
                print_task_status(task_to_run, "basladi");
                
                // RT gorev tamamlanana kadar calistir
                while (task_to_run->remaining_time > 0) {
                    vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM_MS));
                    g_current_time++;
                    task_execute(task_to_run);
                    task_to_run->last_active_time = g_current_time;
                    
                    // Varis kontrolu
                    check_arriving_tasks();
                    
                    // Gorev devam ediyorsa yurutuluyor mesaji
                    if (task_to_run->remaining_time > 0) {
                        print_task_status(task_to_run, "yurutuluyor");
                    }
                    
                    // Timeout kontrolu
                    check_timeouts();
                }
                
                // RT gorev tamamlandi
                task_terminate(task_to_run, g_current_time);
                g_completed_tasks++;
                print_task_status(task_to_run, "sonlandi");
                check_timeouts();
                
                g_context_switches++;
                continue;
            }
        }
        
        // 2. Kullanici gorevlerini kontrol et (MLFQ)
        int queue_index = find_highest_priority_queue();
        
        // RT kuyruk zaten yukarida islendi
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
                task_start(task_to_run, g_current_time);
                print_task_status(task_to_run, "basladi");

                while (task_to_run->remaining_time > 0) {
                    vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM_MS));
                    g_current_time++;
                    task_execute(task_to_run);
                    task_to_run->last_active_time = g_current_time;

                    // Yeni gelenleri ekle
                    check_arriving_tasks();

                    if (task_to_run->remaining_time == 0) {
                        task_terminate(task_to_run, g_current_time);
                        g_completed_tasks++;
                        print_task_status(task_to_run, "sonlandi");
                        break;
                    }

                    // Oncelik dusur (her quantum)
                    demote_priority(task_to_run);

                    // Daha yuksek oncelikli varsa kes
                    int hpq = find_highest_priority_queue();
                    int preempt = 0;
                    if (hpq != -1 && hpq < task_to_run->current_priority) preempt = 1;
                    else if (hpq == task_to_run->current_priority && !queue_is_empty(hpq)) preempt = 1;

                    if (preempt) {
                        task_suspend(task_to_run);
                        print_task_status(task_to_run, "askida");
                        task_resume(task_to_run);
                        queue_add(task_to_run->current_priority, task_to_run);
                        break;
                    } else {
                        // Kesinti yoksa calismaya devam ediyor
                        print_task_status(task_to_run, "yurutuluyor");
                    }
                }

                g_context_switches++;
                continue;
            }
        }
        
        // 3. Calistirilacak gorev yok
        if (g_completed_tasks >= g_task_count) {
            break;
        }
        
        // Bekleme - yeni gorevlerin gelmesini bekle
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
    
    // Simulasyonu sonlandir
    vTaskEndScheduler();
}

// FreeRTOS hook fonksiyonlari

void vApplicationIdleHook(void) {
}

void vApplicationTickHook(void) {
}

// Static allocation icin gerekli
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

// Ana fonksiyon
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Kullanim: %s <giris_dosyasi>\n", argv[0]);
        printf("Ornek: %s giris.txt\n", argv[0]);
        return 1;
    }
    
    // Kuyruklari baslat
    init_queues();
    
    // Gorevleri dosyadan yukle
    if (load_tasks_from_file(argv[1]) <= 0) {
        printf("[HATA] Gorev yuklenemedi!\n");
        return 1;
    }
    
    // Scheduler gorevini olustur
    xTaskCreate(
        vSchedulerTask,
        "Scheduler",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL
    );
    
    // FreeRTOS scheduler'i baslat
    vTaskStartScheduler();
    
    return 0;
}
