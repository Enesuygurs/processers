// 4 Seviyeli Oncelikli Gorev Siralayici - Header Dosyasi

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

// Sabit tanimlamalar
#define MAX_TASKS               200
#define MAX_TASK_TIME           20      // Maksimum gorev suresi (timeout)
#define TIME_QUANTUM_MS         1000    // 1 saniye = 1000 ms
#define MAX_PRIORITY_LEVEL      20
#define COLOR_PALETTE_SIZE      25

// Oncelik seviyeleri (0-3)
#define PRIORITY_REALTIME       0       // Gercek zamanli
#define PRIORITY_HIGH           1       // Yuksek oncelikli kullanici
#define PRIORITY_MEDIUM         2       // Orta oncelikli kullanici
#define PRIORITY_LOW            3       // Dusuk oncelikli kullanici

// Terminal renk kodlari
#define COLOR_RESET             "\033[0m"

// Veri yapilari
// Gorev durum tipleri
typedef enum {
    TASK_STATE_WAITING,      // Gorev henuz gelmedi
    TASK_STATE_READY,        // Gorev kuyrukta bekliyor
    TASK_STATE_RUNNING,      // Gorev calisiyor
    TASK_STATE_SUSPENDED,    // Gorev askida
    TASK_STATE_TERMINATED    // Gorev tamamlandi
} TaskState;

// Gorev tipleri
typedef enum {
    TASK_TYPE_REALTIME,      // Gercek zamanli (Priority 0)
    TASK_TYPE_USER           // Kullanici gorevleri (Priority 1-3)
} TaskType;

// Gorev bilgi yapisi (Task Control Block benzeri)
typedef struct {
    int task_id;                // Gorev kimlik numarasi
    char task_name[32];         // Gorev adi
    int arrival_time;           // Gorev varis zamani
    int original_priority;      // Baslangic onceligi
    int current_priority;       // Guncel oncelik (MLFQ icin degisir)
    int burst_time;             // Toplam CPU suresi
    int remaining_time;         // Kalan sure
    int executed_time;          // Calistirilan sure
    TaskState state;            // Gorev durumu
    TaskType type;              // Gorev tipi (RT/User)
    int start_time;             // Ilk calisma zamani
    int completion_time;        // Tamamlanma zamani
    const char* color_code;     // Terminal renk kodu
    int timeout_printed;        // Timeout mesaji basildi mi
    int last_active_time;       // Son aktif oldugu zaman
} TaskInfo;

// Dinamik oncelik kuyrugu yapisi
typedef struct {
    TaskInfo* tasks[MAX_TASKS]; // Gorev pointer dizisi
    int count;                  // Kuyruktaki gorev sayisi
} DynamicQueue;

// Fonksiyon prototipleri

// Kuyruk yonetim fonksiyonlari
void init_queues(void);                              // Kuyruklari baslat
void queue_add(int priority, TaskInfo* task);        // Kuyruga gorev ekle
TaskInfo* queue_remove(int priority);                // Kuyruktan gorev al
int queue_is_empty(int priority);                    // Kuyruk bos mu kontrol et
int find_highest_priority_queue(void);               // En yuksek oncelikli kuyrugun numarasini bul

// Scheduler yonetim fonksiyonlari
void check_arriving_tasks(void);                     // Yeni gelen gorevleri kontrol et
void check_timeouts(void);                           // Zaman asimi kontrolu
void demote_priority(TaskInfo* task);                // MLFQ: onceligi dusur

// Cikti fonksiyonlari
void print_task_status(TaskInfo* task, const char* status);

// Dosya islemleri
int load_tasks_from_file(const char* filename);      // Dosyadan gorevleri yukle

// Gorev yardimci fonksiyonlari (tasks.c)
const char* get_task_state_string(TaskState state);      // Durum enum'unu string'e cevir
const char* get_task_type_string(TaskType type);          // Tip enum'unu string'e cevir
void print_task_info(TaskInfo* task);                     // Gorev bilgilerini yazdir
void task_start(TaskInfo* task, int current_time);        // Gorevi basla
void task_suspend(TaskInfo* task);                        // Gorevi askiya al
void task_resume(TaskInfo* task);                         // Gorevi devam ettir
void task_terminate(TaskInfo* task, int current_time);    // Gorevi sonlandir
int task_execute(TaskInfo* task);                         // 1 saniye calistir
int task_is_ready(TaskInfo* task, int current_time);      // Gorev hazir mi
int task_is_timeout(TaskInfo* task, int current_time);    // Timeout oldu mu

#endif /* SCHEDULER_H */
