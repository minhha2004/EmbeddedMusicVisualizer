#include "Graphic/graphic.h"
#include "Graphic/main_page/mainpage.h"
#include "Graphic/music_visualizer_pages/mvpage.h"
#include "Graphic/lvgl/lvgl.h"
#include "MusicProcessor/musicprocessor.h"

#include "LedMatrix/led.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

pthread_mutex_t lvgl_mutex;
mv_value_t value;

void* thread0(void* arg) {
    (void)arg;
    processing_function();   // vòng while RECORDING
    return NULL;
}

void* thread1(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&lvgl_mutex);
        if (MusicVisualizerPage && MusicVisualizerPage->state == MV_PAGE_INIT) {
            MusicVisualizerPage->sub_page_main_function(&value);
        } else if (MusicVisualizerPage && MusicVisualizerPage->state == MV_PAGE_DEINIT) {
            MusicVisualizerPage->sub_page_deinit();
        }
        pthread_mutex_unlock(&lvgl_mutex);
        usleep(50000); /* 50ms */
    }
    return NULL;
}

int main(void)
{
    /* Init mutex for LVGL early */
    pthread_mutex_init(&lvgl_mutex, NULL);

    /* Initialize graphics */
    graphic_result_t result = graphic_init();
    if (result != GRAPHIC_OK) {
        printf("Error: Failed to initialize graphics: %d\n", result);
        return -1;
    }

    /* Create UI */
    mainpage_create(lv_scr_act());

    /* Init music processor + start recording */
    mp_init();
    value.value = get_magnitude_data();
    printf("Magnitude data pointer first bin: %f\n", value.value[0]);
    mp_start_recording();

    /* LED MATRIX INIT (SPI MAX7219) */
    if (led_init("/dev/spidev0.0", 3) == 0) {
        // 0 = pattern mode (không cần mp_get_bands32)
        // khi bạn implement mp_get_bands32 thì đổi sang 1
        led_start_thread(1);
        printf("LED matrix started.\n");
    } else {
        printf("Warning: LED init failed (no spidev?). Continue without LED.\n");
    }

    /* Start threads */
    pthread_t t_audio, t_ui;
    if (pthread_create(&t_audio, NULL, thread0, NULL) != 0) {
        printf("Error: cannot create audio thread\n");
        return -1;
    }
    if (pthread_create(&t_ui, NULL, thread1, NULL) != 0) {
        printf("Error: cannot create UI thread\n");
        return -1;
    }

    /* Main loop */
    printf("Starting main loop... (Press Ctrl+C to exit)\n");
    while (1) {
        lv_tick_inc(5);

        pthread_mutex_lock(&lvgl_mutex);
        graphic_task_handler();
        pthread_mutex_unlock(&lvgl_mutex);

        usleep(5000); /* 5ms */
    }

    return 0;
}
