#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// init driver (spidev thường là "/dev/spidev0.0")
int led_init(const char* spidev, int intensity_0_15);

// clear display
void led_clear(void);

// draw spectrum columns: heights[32] each 0..8
// flip_x/flip_y để sửa hướng hiển thị nếu bị ngược
void led_draw_columns(const uint8_t heights[32], int flip_x, int flip_y);

// --- test pattern helpers ---
void led_test_sweep_once(int delay_ms, int flip_x, int flip_y);

// --- background thread (optional) ---
int led_start_thread(int use_fft); // use_fft=0: chạy pattern, use_fft=1: ăn FFT (cần mp_get_bands32)
void led_stop_thread(void);

#ifdef __cplusplus
}
#endif
