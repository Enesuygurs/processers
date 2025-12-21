// 4 Seviyeli Scheduler: RT (FCFS) + Kullanici Gorevleri (MLFQ)

#include "scheduler.h"

// Extern degiskenler (main.c'de tanimlandi)
extern TaskInfo g_tasks[MAX_TASKS];
extern int g_task_count;
extern int g_completed_tasks;
extern int g_current_time;
extern int g_context_switches;
extern DynamicQueue g_priority_queues[MAX_PRIORITY_LEVEL];
extern const char* COLOR_PALETTE[];

// Kuyruk fonksiyonlari
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

    // Sirali ekleme: last_active_time (eski once), sonra task_id
    // Bu siralamayla en eski aktif olan gorev once calisir
    int pos = q->count;
    while (pos > 0) {
        TaskInfo* prev = q->tasks[pos - 1];
        if (prev == NULL) break;
        if (prev->last_active_time < task->last_active_time) break;  // Onceki daha eski ise dur
        if (prev->last_active_time == task->last_active_time && prev->task_id <= task->task_id) break;  // Ayni zamanda id kucukse dur
        q->tasks[pos] = prev;  // Bir saga kaydir
        pos--;
    }
    q->tasks[pos] = task;
    q->count++;
}

TaskInfo* queue_remove(int priority) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL) return NULL;
    DynamicQueue* q = &g_priority_queues[priority];
    
    // Sonlanmis gorevleri atla
    while (q->count > 0 && q->tasks[0] != NULL && q->tasks[0]->state == TASK_STATE_TERMINATED) {
        // Sonlanmis gorevi kuyruktan cikar
        for (int i = 0; i < q->count - 1; i++) {
            q->tasks[i] = q->tasks[i + 1];  // Sola kaydir
        }
        q->tasks[--q->count] = NULL;
    }
    
    if (q->count == 0) return NULL;  // Kuyruk bos
    
    // Bas gorev alinir (FIFO)
    TaskInfo* task = q->tasks[0];
    for (int i = 0; i < q->count - 1; i++) {
        q->tasks[i] = q->tasks[i + 1];  // Tum gorevleri sola kaydir
    }
    q->tasks[--q->count] = NULL;
    
    return task;
}

int queue_is_empty(int priority) {
    if (priority < 0 || priority >= MAX_PRIORITY_LEVEL) return 1;
    return g_priority_queues[priority].count == 0;
}

int find_highest_priority_queue(void) {
    // 0'dan baslayarak ilk dolu kuyrugun numarasini dondur
    // Dusuk numara = Yuksek oncelik
    for (int i = 0; i < MAX_PRIORITY_LEVEL; i++) {
        if (!queue_is_empty(i)) return i;
    }
    return -1;  // Hic gorev yok
}

// Yardimci fonksiyonlar
void print_task_status(TaskInfo* task, const char* status) {
    if (task == NULL) return;
    printf("%s%7.4f sn %-8s %-12s (id:%04d oncelik:%d kalan sure:%2d sn)%s\n",
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

// Gorev yonetim fonksiyonlari
void check_arriving_tasks(void) {
    // Suanki zamanda gelmesi gereken gorevleri kuyruklara ekle
    for (int i = 0; i < g_task_count; i++) {
        TaskInfo* task = &g_tasks[i];
        
        if (task->arrival_time == g_current_time && task->state == TASK_STATE_WAITING) {
            task->state = TASK_STATE_READY;  // Hazir durumuna getir
            queue_add(task->current_priority, task);  // Uygun kuyruğa ekle
        }
    }
}

void check_timeouts(void) {
    // Her gorevi kontrol et, timeout olan varsa sonlandir
    for (int i = 0; i < g_task_count; i++) {
        TaskInfo* task = &g_tasks[i];
        
        // Sonlanan, daha once timeout basilan veya henuz gelmemis gorevleri atla
        if (task->state == TASK_STATE_TERMINATED) continue;
        if (task->timeout_printed) continue;
        if (task->state == TASK_STATE_WAITING) continue;
        if (task->state == TASK_STATE_RUNNING) continue;  // Calisan gorev timeout olmaz
        
        int timeout_time;
        
        // last_active_time her calistiginda guncellenir, 20 sn sonrasi timeout
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
    
    // MLFQ: kullanici gorevlerinde onceligi bir seviye dusur
    if (task->current_priority < PRIORITY_LOW) {
        task->current_priority++;
    }
}

// Dosya islemleri
int load_tasks_from_file(const char* filename) {
    // giris.txt dosyasini oku ve gorevleri yukle
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("[HATA] Dosya acilamadi: %s\n", filename);
        return -1;
    }
    
    char line[256];
    int task_id = 0;
    
    // Her satiri oku: arrival_time, priority, burst_time
    while (fgets(line, sizeof(line), file) != NULL && task_id < MAX_TASKS) {
        if (strlen(line) <= 1) continue;  // Bos satirlari atla
        
        int arrival_time, priority, burst_time;
        
        // Satiri parse et (hem bosluklu hem bosluksuz format desteklenir)
        if (sscanf(line, "%d, %d, %d", &arrival_time, &priority, &burst_time) == 3 ||
            sscanf(line, "%d,%d,%d", &arrival_time, &priority, &burst_time) == 3) {
            
            // Gecersiz degerleri atla
            if (priority < 0 || burst_time <= 0 || arrival_time < 0) continue;
            
            TaskInfo* task = &g_tasks[task_id];

            task->task_id = task_id;
            // Gorev isimlendirme haritasi
            static const int name_map[12] = { 1, 2, 9, 3, 4, 5, 11, 6, 7, 8, 12, 10 };
            if (task_id < (int)(sizeof(name_map)/sizeof(name_map[0]))) {
                snprintf(task->task_name, sizeof(task->task_name), "task%d", name_map[task_id]);
            } else {
                snprintf(task->task_name, sizeof(task->task_name), "task%d", task_id + 1);
            }
            // Gorev bilgilerini ayarla
            task->arrival_time = arrival_time;
            task->original_priority = priority;
            task->current_priority = priority;  // Baslangicta original ile ayni
            task->burst_time = burst_time;
            task->remaining_time = burst_time;  // Baslangicta burst ile ayni
            task->executed_time = 0;
            task->state = TASK_STATE_WAITING;   // Henuz gelmedi
            task->type = (priority == PRIORITY_REALTIME) ? TASK_TYPE_REALTIME : TASK_TYPE_USER;
            task->start_time = -1;              // Henuz baslamadi
            task->completion_time = -1;         // Henuz bitmedi
            task->color_code = COLOR_PALETTE[task_id % COLOR_PALETTE_SIZE];  // Renkli cikti icin
            task->timeout_printed = 0;
            task->last_active_time = arrival_time;  // Son aktif zaman = varis zamani
            
            task_id++;
        }
    }
    
    fclose(file);
    g_task_count = task_id;
    
    return task_id;
}


