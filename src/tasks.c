/**
 * @file tasks.c
 * @brief FreeRTOS Gorev Simulasyon Fonksiyonlari
 * 
 * Bu dosya, gorev yaratma ve yonetim fonksiyonlarini icerir.
 * Ana scheduler mantigi main.c'de, kuyruk islemleri scheduler.c'de
 * 
 * @author Isletim Sistemleri Dersi Projesi
 * @date 2025
 */

#include "scheduler.h"

/*=============================================================================
 * GOREV YARDIMCI FONKSIYONLARI
 *============================================================================*/

/**
 * @brief Gorev durumunu string olarak dondurur
 * @param state Gorev durumu
 * @return Durum stringi
 */
const char* get_task_state_string(TaskState state) {
    switch (state) {
        case TASK_STATE_WAITING:    return "BEKLIYOR";
        case TASK_STATE_READY:      return "HAZIR";
        case TASK_STATE_RUNNING:    return "CALISIYOR";
        case TASK_STATE_SUSPENDED:  return "ASKIDA";
        case TASK_STATE_TERMINATED: return "SONLANDI";
        default:                    return "BILINMIYOR";
    }
}

/**
 * @brief Gorev tipini string olarak dondurur
 * @param type Gorev tipi
 * @return Tip stringi
 */
const char* get_task_type_string(TaskType type) {
    switch (type) {
        case TASK_TYPE_REALTIME: return "GERCEK ZAMANLI";
        case TASK_TYPE_USER:     return "KULLANICI";
        default:                 return "BILINMIYOR";
    }
}

/**
 * @brief Gorev bilgilerini ekrana yazdirir (debug icin)
 * @param task Gorev bilgisi
 */
void print_task_info(TaskInfo* task) {
    if (task == NULL) return;
    
    printf("Gorev ID: %d\n", task->task_id);
    printf("  Varis Zamani: %d\n", task->arrival_time);
    printf("  Oncelik: %d\n", task->current_priority);
    printf("  Kalan Sure: %d sn\n", task->remaining_time);
    printf("  Durum: %s\n", get_task_state_string(task->state));
    printf("  Tip: %s\n", get_task_type_string(task->type));
}

/**
 * @brief Gorevi baslat
 * @param task Gorev bilgisi
 * @param current_time Mevcut zaman
 */
void task_start(TaskInfo* task, int current_time) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_RUNNING;
    if (task->start_time == -1) {
        task->start_time = current_time;
    }
}

/**
 * @brief Gorevi askiya al
 * @param task Gorev bilgisi
 */
void task_suspend(TaskInfo* task) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_SUSPENDED;
}

/**
 * @brief Gorevi devam ettir
 * @param task Gorev bilgisi
 */
void task_resume(TaskInfo* task) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_READY;
}

/**
 * @brief Gorevi sonlandir
 * @param task Gorev bilgisi
 * @param current_time Mevcut zaman
 */
void task_terminate(TaskInfo* task, int current_time) {
    if (task == NULL) return;
    
    task->state = TASK_STATE_TERMINATED;
    task->completion_time = current_time;
}

/**
 * @brief Gorev calistir (1 saniye)
 * @param task Gorev bilgisi
 * @return Kalan sure
 */
int task_execute(TaskInfo* task) {
    if (task == NULL) return -1;
    
    task->remaining_time--;
    task->executed_time++;
    
    return task->remaining_time;
}

/**
 * @brief Gorev hazir mi kontrol et
 * @param task Gorev bilgisi
 * @param current_time Mevcut zaman
 * @return 1: hazir, 0: degil
 */
int task_is_ready(TaskInfo* task, int current_time) {
    if (task == NULL) return 0;
    
    return (task->arrival_time <= current_time && 
            task->state != TASK_STATE_TERMINATED &&
            task->remaining_time > 0);
}

/**
 * @brief Gorev timeout oldu mu kontrol et
 * @param task Gorev bilgisi
 * @param current_time Mevcut zaman
 * @return 1: timeout, 0: degil
 */
int task_is_timeout(TaskInfo* task, int current_time) {
    if (task == NULL) return 0;
    
    int timeout_time = task->arrival_time + MAX_TASK_TIME;
    return (current_time >= timeout_time && task->state != TASK_STATE_TERMINATED);
}
