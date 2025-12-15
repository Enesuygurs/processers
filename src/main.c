/**
 * @file main.c
 * @brief FreeRTOS Gorev Siralayici Simulasyonu - Ana Program
 */

#include "scheduler.h"
#include <signal.h>

static Scheduler* g_scheduler = NULL;

void signal_handler(int signum) {
    (void)signum;
    if (g_scheduler != NULL) {
        scheduler_stop(g_scheduler);
        scheduler_destroy(g_scheduler);
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Kullanim: %s <giris_dosyasi>\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    Scheduler* scheduler = scheduler_create();
    if (scheduler == NULL) {
        printf("[HATA] Scheduler olusturulamadi!\n");
        return 1;
    }
    
    g_scheduler = scheduler;
    
    int task_count = load_tasks_from_file(scheduler, input_file);
    
    if (task_count <= 0) {
        printf("[HATA] Hic gorev yuklenemedi!\n");
        scheduler_destroy(scheduler);
        return 1;
    }
    
    scheduler_run(scheduler);
    
    scheduler_destroy(scheduler);
    g_scheduler = NULL;
    
    return 0;
}
