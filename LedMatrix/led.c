#include "led.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// Nếu bạn đã implement mp_get_bands32 thì mở include này.
// Nếu chưa làm FFT->bands32, cứ để comment và chạy use_fft=0 (pattern).
#include "musicprocessor.h"

static int g_fd = -1;
static const int g_n = 4;          // 8x32 => 4 chips
static uint8_t g_fb[4][8];         // [device][row]
static int g_flip_x = 0, g_flip_y = 0;

static pthread_t g_th;
static volatile int g_run = 0;
static volatile int g_use_fft = 0;

static void send_all(uint8_t reg, uint8_t val) {
    uint8_t tx[8];
    for (int i = 0; i < g_n; i++) { tx[i*2] = reg; tx[i*2+1] = val; }
    (void)write(g_fd, tx, g_n * 2);
}

// GỬI 1 ROW cho cả 4 chip.
// Đảo chain để đúng thứ tự thực tế (chip xa Pi nhận trước).
static void send_row_all(int addr_1_8, const uint8_t data_per_dev[4]) {
    uint8_t tx[8];
    for (int i = 0; i < g_n; i++) {
        //int dev = (g_n - 1) - i;
        int dev = i;            // <-- quan trọng
        tx[i*2 + 0] = (uint8_t)addr_1_8;
        tx[i*2 + 1] = data_per_dev[dev];
    }
    (void)write(g_fd, tx, g_n * 2);
}

int led_init(const char* spidev, int intensity_0_15) {
    g_fd = open(spidev, O_RDWR);
    if (g_fd < 0) {
        perror("open spidev");
        return -1;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 2000000; // 2MHz

    ioctl(g_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(g_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(g_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    send_all(0x0F, 0x00); // display test off
    send_all(0x09, 0x00); // no decode
    send_all(0x0B, 0x07); // scan 8 rows
    send_all(0x0C, 0x01); // normal operation

    if (intensity_0_15 < 0) intensity_0_15 = 0;
    if (intensity_0_15 > 15) intensity_0_15 = 15;
    send_all(0x0A, (uint8_t)intensity_0_15);

    led_clear();
    return 0;
}

void led_clear(void) {
    memset(g_fb, 0, sizeof(g_fb));
    for (int row = 1; row <= 8; row++) {
        uint8_t d[4] = {0,0,0,0};
        send_row_all(row, d);
    }
}

void led_draw_columns(const uint8_t heights[32], int flip_x, int flip_y) {
    memset(g_fb, 0, sizeof(g_fb));

    for (int x = 0; x < 32; x++) {
        int xx = flip_x ? (31 - x) : x;
        uint8_t h = heights[x] > 8 ? 8 : heights[x];

        for (int yy = 0; yy < h; yy++) {
            int y = 7 - yy;               // bottom-up
            if (flip_y) y = 7 - y;

            int dev = xx / 8;             // 0..3
            int col = xx % 8;             // 0..7
            g_fb[dev][y] |= (1u << (7 - col));
        }
    }

    // flush each row
    for (int row = 0; row < 8; row++) {
        uint8_t d[4];
        for (int dev = 0; dev < 4; dev++) d[dev] = g_fb[dev][row];
        send_row_all(row + 1, d);
    }
}

void led_test_sweep_once(int delay_ms, int flip_x, int flip_y) {
    uint8_t h[32] = {0};
    for (int x = 0; x < 32; x++) {
        memset(h, 0, sizeof(h));
        h[x] = 8;
        led_draw_columns(h, flip_x, flip_y);
        usleep(delay_ms * 1000);
    }
}

static uint8_t to_h(float v01) {
    if (v01 < 0) v01 = 0;
    if (v01 > 1) v01 = 1;
    int h = (int)lroundf(v01 * 8.0f);
    if (h < 0) h = 0;
    if (h > 8) h = 8;
    return (uint8_t)h;
}

static void* thread_fn(void* _) {
    (void)_;
    uint8_t heights[32];
    float bands[32];

    while (g_run) {
        if (!g_use_fft) {
            // Pattern mode
            led_test_sweep_once(40, g_flip_x, g_flip_y);
        } else {
            // FFT mode: cần mp_get_bands32()
            mp_get_bands32(bands);
            for (int i = 0; i < 32; i++) heights[i] = to_h(bands[i]);
            led_draw_columns(heights, g_flip_x, g_flip_y);
            usleep(33000); // ~30fps
        }
    }
    return NULL;
}

int led_start_thread(int use_fft) {
    if (g_fd < 0) return -1;
    g_use_fft = use_fft ? 1 : 0;
    g_run = 1;
    return pthread_create(&g_th, NULL, thread_fn, NULL);
}

void led_stop_thread(void) {
    if (!g_run) return;
    g_run = 0;
    pthread_join(g_th, NULL);
}
