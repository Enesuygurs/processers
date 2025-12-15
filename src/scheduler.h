/**
 * @file scheduler.h
 * @brief 4 Seviyeli Oncelikli Gorev Sıralayici (Scheduler) Header Dosyasi
 * 
 * Bu dosya gorevlendirici (scheduler) icin gerekli veri yapilarini,
 * kuyruk tanimlamalarini ve fonksiyon prototiplerini icerir.
 * 
 * Sistem 4 oncelik seviyesinde calisir:
 * - Seviye 0: Gercek Zamanli (Real-Time) - FCFS algoritmasi
 * - Seviye 1: Yuksek Oncelikli Kullanici Gorevi
 * - Seviye 2: Orta Oncelikli Kullanici Gorevi  
 * - Seviye 3: Dusuk Oncelikli Kullanici Gorevi
 * 
 * Kullanici gorevleri (1-3) Multi-Level Feedback Queue (MLFQ) ile yonetilir.
 * 
 * @author Isletim Sistemleri Dersi Projesi
 * @date 2025
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/*=============================================================================
 * SABIT TANIMLAMALARI
 *============================================================================*/

/* Oncelik seviyeleri */
#define PRIORITY_REALTIME       0   /* Gercek zamanli gorev (en yuksek) */
#define PRIORITY_HIGH           1   /* Yuksek oncelikli kullanici gorevi */
#define PRIORITY_MEDIUM         2   /* Orta oncelikli kullanici gorevi */
#define PRIORITY_LOW            3   /* Dusuk oncelikli kullanici gorevi (en dusuk) */

#define NUM_PRIORITY_LEVELS     4   /* Toplam oncelik seviye sayisi */
#define NUM_USER_QUEUES         3   /* Kullanici kuyruk sayisi (1, 2, 3) */

/* Zaman kuantumlari (saniye cinsinden) */
#define TIME_QUANTUM            1   /* Temel zaman kuantumu */
#define MAX_TASK_TIME           20  /* Maksimum gorev suresi (timeout) */

/* Kuyruk boyutlari */
#define MAX_QUEUE_SIZE          100 /* Maksimum kuyruk eleman sayisi */
#define MAX_TASKS               200 /* Maksimum toplam gorev sayisi */

/* Terminal renk kodlari (ANSI) */
#define COLOR_RESET             "\033[0m"
#define COLOR_RED               "\033[31m"
#define COLOR_GREEN             "\033[32m"
#define COLOR_YELLOW            "\033[33m"
#define COLOR_BLUE              "\033[34m"
#define COLOR_MAGENTA           "\033[35m"
#define COLOR_CYAN              "\033[36m"
#define COLOR_WHITE             "\033[37m"
#define COLOR_BOLD              "\033[1m"
#define COLOR_BG_RED            "\033[41m"
#define COLOR_BG_GREEN          "\033[42m"
#define COLOR_BG_YELLOW         "\033[43m"
#define COLOR_BG_BLUE           "\033[44m"
#define COLOR_BG_MAGENTA        "\033[45m"
#define COLOR_BG_CYAN           "\033[46m"

/*=============================================================================
 * VERI YAPILARI
 *============================================================================*/

/**
 * @brief Gorev Durumu Enum
 * 
 * Bir gorevin yasam dongusu boyunca alabilecegi durumlar
 */
typedef enum {
    TASK_STATE_WAITING,     /* Kuyrukta bekliyor, henuz calistirilmadi */
    TASK_STATE_READY,       /* Calismaya hazir */
    TASK_STATE_RUNNING,     /* Suanda calisıyor */
    TASK_STATE_SUSPENDED,   /* Askiya alindi (preempted) */
    TASK_STATE_TERMINATED   /* Sonlandirildi */
} TaskState;

/**
 * @brief Gorev Tipi Enum
 * 
 * Gorevin gercek zamanli mi yoksa kullanici gorevi mi oldugunu belirtir
 */
typedef enum {
    TASK_TYPE_REALTIME,     /* Gercek zamanli gorev (oncelik 0) */
    TASK_TYPE_USER          /* Kullanici gorevi (oncelik 1-3) */
} TaskType;

/**
 * @brief Gorev Kontrol Blogu (TCB - Task Control Block)
 * 
 * Her gorev icin gereken tum bilgileri icerir.
 * Bu yapi FreeRTOS'taki TCB yapisina benzer sekilde tasarlanmistir.
 */
typedef struct Task {
    /* Gorev Kimlik Bilgileri */
    int task_id;                    /* Benzersiz gorev kimlik numarasi */
    char task_name[32];             /* Gorev adi */
    
    /* Zamanlama Bilgileri */
    int arrival_time;               /* Varis zamani (saniye) */
    int original_priority;          /* Baslangic oncelik degeri (0-3) */
    int current_priority;           /* Mevcut oncelik degeri (MLFQ ile degisebilir) */
    int burst_time;                 /* Toplam gerekli CPU suresi (saniye) */
    int remaining_time;             /* Kalan CPU suresi (saniye) */
    int executed_time;              /* Simdiye kadar calistirilan sure */
    
    /* Durum Bilgileri */
    TaskState state;                /* Gorevin mevcut durumu */
    TaskType type;                  /* Gorev tipi (RT veya User) */
    
    /* Istatistik Bilgileri */
    int start_time;                 /* Gorevin ilk calistirildigi zaman */
    int completion_time;            /* Gorevin tamamlandigi zaman */
    int waiting_time;               /* Toplam bekleme suresi */
    int turnaround_time;            /* Donus suresi (completion - arrival) */
    int response_time;              /* Ilk cevap suresi (first_run - arrival) */
    
        /* Zamanaşımı mesajı yalnızca bir kez basılsın diye bayrak */
        bool timeout_printed;
    
    /* Gorsel Bilgiler */
    const char* color_code;         /* Terminal renk kodu */
    
    /* Bagli Liste Isaretcileri */
    struct Task* next;              /* Sonraki gorev (kuyruk icin) */
    struct Task* prev;              /* Onceki gorev (cift bagli liste) */
} Task;

/**
 * @brief Kuyruk Yapisi (Queue Structure)
 * 
 * FIFO (First-In-First-Out) kuyruk yapisi.
 * Her oncelik seviyesi icin ayri bir kuyruk kullanilir.
 */
typedef struct Queue {
    Task* front;            /* Kuyrugun basi (ilk eleman) */
    Task* rear;             /* Kuyrugun sonu (son eleman) */
    int count;              /* Kuyruktaki eleman sayisi */
    int priority_level;     /* Bu kuyrugun oncelik seviyesi */
    pthread_mutex_t lock;   /* Thread-safe erisim icin mutex */
} Queue;

/**
 * @brief Gorevlendirici Yapisi (Scheduler Structure)
 * 
 * Tum sıralayıcı bilgilerini ve kuyruklari icerir.
 */
typedef struct Scheduler {
    /* Kuyruklar */
    Queue* realtime_queue;          /* Gercek zamanli gorev kuyrugu (FCFS) */
    Queue* feedback_queues[NUM_USER_QUEUES];  /* Geri beslemeli kuyruklar (MLFQ) */
    
    /* Gorev Yonetimi */
    Task* all_tasks[MAX_TASKS];     /* Tum gorevlerin listesi */
    int total_tasks;                /* Toplam gorev sayisi */
    int completed_tasks;            /* Tamamlanan gorev sayisi */
    
    /* Sistem Durumu */
    Task* current_task;             /* Suanda calisan gorev */
    int current_time;               /* Mevcut sistem zamani (saniye) */
    bool is_running;                /* Scheduler calisıyor mu? */
    
    /* Senkronizasyon */
    pthread_mutex_t scheduler_lock; /* Scheduler mutex */
    
    /* Istatistikler */
    int context_switches;           /* Baglam degisim sayisi */
    int total_idle_time;            /* Toplam bos zaman */
} Scheduler;

/**
 * @brief Giris Dosyasi Gorev Satirı
 * 
 * giris.txt dosyasindan okunan her satir icin
 */
typedef struct TaskInput {
    int arrival_time;       /* Varis zamani */
    int priority;           /* Oncelik (0-3) */
    int burst_time;         /* Gorev suresi */
} TaskInput;

/*=============================================================================
 * FONKSIYON PROTOTIPLERI
 *============================================================================*/

/* --- Scheduler Yonetim Fonksiyonlari --- */

/**
 * @brief Yeni bir scheduler olusturur ve baslatir
 * @return Olusturulan scheduler isaretcisi, hata durumunda NULL
 */
Scheduler* scheduler_create(void);

/**
 * @brief Scheduler'i ve tum kaynaklari serbest birakir
 * @param scheduler Serbest birakilacak scheduler
 */
void scheduler_destroy(Scheduler* scheduler);

/**
 * @brief Scheduler'i calistirir (ana dongu)
 * @param scheduler Calistirilacak scheduler
 */
void scheduler_run(Scheduler* scheduler);

/**
 * @brief Scheduler'i durdurur
 * @param scheduler Durdurulacak scheduler
 */
void scheduler_stop(Scheduler* scheduler);

/* --- Gorev Yonetim Fonksiyonlari --- */

/**
 * @brief Yeni bir gorev olusturur
 * @param arrival_time Varis zamani
 * @param priority Oncelik seviyesi (0-3)
 * @param burst_time Gerekli CPU suresi
 * @return Olusturulan gorev isaretcisi
 */
Task* task_create(int arrival_time, int priority, int burst_time);

/**
 * @brief Gorevi serbest birakir
 * @param task Serbest birakilacak gorev
 */
void task_destroy(Task* task);

/**
 * @brief Gorevi uygun kuyruga ekler
 * @param scheduler Scheduler isaretcisi
 * @param task Eklenecek gorev
 */
void task_enqueue(Scheduler* scheduler, Task* task);

/**
 * @brief Gorevi askiya alir (suspend)
 * @param scheduler Scheduler isaretcisi
 * @param task Askiya alinacak gorev
 */
void task_suspend(Scheduler* scheduler, Task* task);

/**
 * @brief Askiya alinan gorevi devam ettirir (resume)
 * @param scheduler Scheduler isaretcisi
 * @param task Devam ettirilecek gorev
 */
void task_resume(Scheduler* scheduler, Task* task);

/**
 * @brief Gorevi sonlandirir (terminate)
 * @param scheduler Scheduler isaretcisi
 * @param task Sonlandirilacak gorev
 */
void task_terminate(Scheduler* scheduler, Task* task);

/**
 * @brief Gorevin bir zaman dilimini calistirir
 * @param scheduler Scheduler isaretcisi
 * @param task Calistirilacak gorev
 * @param time_slice Zaman dilimi (saniye)
 */
void task_execute(Scheduler* scheduler, Task* task, int time_slice);

/* --- Kuyruk Yonetim Fonksiyonlari --- */

/**
 * @brief Yeni bir kuyruk olusturur
 * @param priority_level Kuyrugun oncelik seviyesi
 * @return Olusturulan kuyruk isaretcisi
 */
Queue* queue_create(int priority_level);

/**
 * @brief Kuyrugu serbest birakir
 * @param queue Serbest birakilacak kuyruk
 */
void queue_destroy(Queue* queue);

/**
 * @brief Kuyruga gorev ekler (enqueue - FIFO)
 * @param queue Hedef kuyruk
 * @param task Eklenecek gorev
 */
void queue_enqueue(Queue* queue, Task* task);

/**
 * @brief Kuyruktan gorev cikarir (dequeue - FIFO)
 * @param queue Kaynak kuyruk
 * @return Cikarilan gorev, bos ise NULL
 */
Task* queue_dequeue(Queue* queue);

/**
 * @brief Kuyrugun bos olup olmadigini kontrol eder
 * @param queue Kontrol edilecek kuyruk
 * @return true ise bos, false ise dolu
 */
bool queue_is_empty(Queue* queue);

/**
 * @brief Kuyruktaki eleman sayisini dondurur
 * @param queue Kontrol edilecek kuyruk
 * @return Eleman sayisi
 */
int queue_size(Queue* queue);

/**
 * @brief Kuyrugun basindaki gorevi dondurur (cikmadan)
 * @param queue Kaynak kuyruk
 * @return Bastaki gorev, bos ise NULL
 */
Task* queue_peek(Queue* queue);

/* --- Dosya Islemleri --- */

/**
 * @brief Giris dosyasini okur ve gorevleri yukler
 * @param scheduler Scheduler isaretcisi
 * @param filename Dosya adi
 * @return Basarili ise 0, hata ise -1
 */
int load_tasks_from_file(Scheduler* scheduler, const char* filename);

/* --- Yardimci Fonksiyonlar --- */

/**
 * @brief Gorev icin rastgele renk kodu atar
 * @param task Renk atanacak gorev
 */
void assign_random_color(Task* task);

/**
 * @brief Gorevin bilgilerini yazdirir
 * @param task Yazdirilacak gorev
 * @param message Ek mesaj
 */
void print_task_info(Task* task, const char* message);

/**
 * @brief Scheduler durumunu yazdirir
 * @param scheduler Yazdirilacak scheduler
 */
void print_scheduler_status(Scheduler* scheduler);

/**
 * @brief Kuyruk icerigini yazdirir
 * @param queue Yazdirilacak kuyruk
 * @param queue_name Kuyruk adi
 */
void print_queue_status(Queue* queue, const char* queue_name);

/**
 * @brief Istatistikleri yazdirir
 * @param scheduler Scheduler isaretcisi
 */
void print_statistics(Scheduler* scheduler);

/**
 * @brief Zaman damgasi ile mesaj yazdirir
 * @param scheduler Scheduler isaretcisi
 * @param format Printf format stringi
 * @param ... Degisken argumanlar
 */
void log_message(Scheduler* scheduler, const char* format, ...);

/**
 * @brief Gercek zamanli kuyrugunu kontrol eder
 * @param scheduler Scheduler isaretcisi
 * @return Bekleyen RT gorev var ise true
 */
bool has_pending_realtime_tasks(Scheduler* scheduler);

/**
 * @brief En yuksek oncelikli bos olmayan kuyrugu bulur
 * @param scheduler Scheduler isaretcisi
 * @return Kuyruk indeksi (0-2), hepsi bos ise -1
 */
int find_highest_priority_queue(Scheduler* scheduler);

/**
 * @brief Belirli bir zamanda varacak gorevleri kontrol eder
 * @param scheduler Scheduler isaretcisi
 * @param time Kontrol edilecek zaman
 */
void check_arriving_tasks(Scheduler* scheduler, int time);

/**
 * @brief MLFQ'da onceligi dusurur
 * @param scheduler Scheduler isaretcisi
 * @param task Onceligi dusurulecek gorev
 * @param force_demote True ise onceligi dusur, false ise dusurme
 */
void demote_priority(Scheduler* scheduler, Task* task, bool force_demote);

/**
 * @brief Baslangic bannerini yazdirir
 */
void print_startup_banner(void);

/**
 * @brief Algoritma bilgilerini yazdirir
 */
void print_algorithm_info(void);

/**
 * @brief Gantt semasini yazdirir
 * @param scheduler Scheduler isaretcisi
 */
void print_gantt_chart(Scheduler* scheduler);

#endif /* SCHEDULER_H */
