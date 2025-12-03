// ==========================================
// FILE: peakmeter.c
// STYLE: LED Segment Mirror (Khối LED kỹ thuật số)
// ==========================================
#include "../music_visualizer_pages/mvpage.h" 
#include "peakmeter.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <graphic.h>

extern mv_page_t PeakMeterPage;
LV_IMG_DECLARE(back_icon_png);

// --- CẤU HÌNH ---
#define BAR_COUNT 48       // Giảm số cột để LED to và rõ hơn
#define SEGMENT_HEIGHT 6   // Chiều cao mỗi viên LED
#define SEGMENT_GAP 2      // Khe hở giữa các viên LED
#define PEAK_GRAVITY 2.0f  
#define PEAK_HOLD_TIME 15  

static int canvas_w = GRAPHIC_HOR_RES;
static int canvas_h = GRAPHIC_VER_RES; 

static lv_obj_t *peak_cont = NULL;
static lv_obj_t *canvas = NULL;
static lv_obj_t *back_btn = NULL;
static lv_color_t *cbuf = NULL;

static float bar_heights[BAR_COUNT];
static float peak_levels[BAR_COUNT];
static int peak_hold_timers[BAR_COUNT];

static void back_event_handler(lv_event_t *e) {
    (void)e;
    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;
    if (cbuf) { free(cbuf); cbuf = NULL; }
    lv_obj_clean(lv_scr_act());
    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(lv_scr_act());
}

mv_page_err_code PeakMeter_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;
    PeakMeterPage.state = MV_PAGE_INIT;

    peak_cont = lv_obj_create(parent);
    lv_obj_set_size(peak_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(peak_cont, lv_color_hex(0x000000), 0); // Đen tuyền
    lv_obj_clear_flag(peak_cont, LV_OBJ_FLAG_SCROLLABLE);

    canvas = lv_canvas_create(peak_cont);
    lv_obj_set_size(canvas, canvas_w, canvas_h);
    lv_obj_center(canvas);

    cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_w, canvas_h));
    if (!cbuf) return MV_PAGE_RET_FAIL;
    
    lv_canvas_set_buffer(canvas, cbuf, canvas_w, canvas_h, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    for(int i=0; i<BAR_COUNT; i++) {
        bar_heights[i] = 0.0f;
        peak_levels[i] = 0.0f;
        peak_hold_timers[i] = 0;
    }

    back_btn = lv_btn_create(peak_cont);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, back_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, &back_icon_png);
    lv_obj_center(back_icon);

    return MV_PAGE_RET_OK;
}

mv_page_err_code PeakMeter_sub_page_deinit(void) {
    if (canvas) { lv_obj_del(canvas); canvas = NULL; }
    if (peak_cont) { lv_obj_del(peak_cont); peak_cont = NULL; }
    if (cbuf) { free(cbuf); cbuf = NULL; }
    return MV_PAGE_RET_OK;
}

// --- UPDATE (LED STYLE) ---
mv_page_err_code PeakMeter_sub_page_main_function(mv_value_t *value) {
    if (!canvas || !value || !value->value) return MV_PAGE_RET_FAIL;

    lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    lv_draw_rect_dsc_t led_dsc;
    lv_draw_rect_dsc_init(&led_dsc);
    
    lv_draw_rect_dsc_t peak_dsc;
    lv_draw_rect_dsc_init(&peak_dsc);
    peak_dsc.bg_color = lv_color_hex(0xFFFFFF); // Peak màu trắng

    int center_y = canvas_h / 2;
    float bar_width_float = (float)canvas_w / (float)BAR_COUNT;
    int bar_w = (int)bar_width_float - 6; // Khe hở ngang rộng hơn (6px)
    if (bar_w < 2) bar_w = 2;

    int max_h = (canvas_h / 2) - 20;

    for (int i = 0; i < BAR_COUNT; i++) {
        int input_idx = i * (BAR_NUMBER / BAR_COUNT); 
        float raw_val = value->value[input_idx];
        if (raw_val < 0) raw_val = -raw_val;

        // Smoothing (Nhanh hơn chút cho LED nảy)
        if (raw_val > bar_heights[i]) {
            bar_heights[i] = bar_heights[i] * 0.5f + raw_val * 0.5f;
        } else {
            bar_heights[i] = bar_heights[i] * 0.8f + raw_val * 0.2f;
        }

        // Tính chiều cao (Sensitivity cho tín hiệu to)
        int h = (int)(bar_heights[i] * 0.04f * (float)max_h);
        if (h > max_h) h = max_h;
        if (h < 0) h = 0;

        // Logic Peak Hold
        if (h >= peak_levels[i]) {
            peak_levels[i] = (float)h;
            peak_hold_timers[i] = PEAK_HOLD_TIME;
        } else {
            if (peak_hold_timers[i] > 0) peak_hold_timers[i]--;
            else {
                peak_levels[i] -= PEAK_GRAVITY;
                if (peak_levels[i] < h) peak_levels[i] = (float)h;
            }
        }

        int x = (int)(i * bar_width_float) + 3;

        // --- VẼ CÁC VIÊN LED (SEGMENTS) ---
        // Thay vì vẽ 1 thanh dài, ta vẽ vòng lặp các viên nhỏ
        for (int y = 0; y < h; y += (SEGMENT_HEIGHT + SEGMENT_GAP)) {
            
            // Tính toán màu sắc dựa trên độ cao (Zone Color)
            // Dưới thấp: Xanh (Hue 120) -> Giữa: Vàng (Hue 60) -> Cao: Đỏ (Hue 0)
            float percent = (float)y / (float)max_h; // 0.0 -> 1.0
            
            // Xanh (120) -> Đỏ (0)
            int hue = (int)(120.0f * (1.0f - percent)); 
            if (hue < 0) hue = 0;
            
            led_dsc.bg_color = lv_color_hsv_to_rgb(hue, 100, 100);

            // Vẽ viên LED trên
            lv_canvas_draw_rect(canvas, x, center_y - y - SEGMENT_HEIGHT, bar_w, SEGMENT_HEIGHT, &led_dsc);
            
            // Vẽ viên LED dưới (Đối xứng)
            // Giảm độ sáng cho phần phản chiếu dưới nước (cho nghệ)
            lv_color_t mirror_col = lv_color_hsv_to_rgb(hue, 100, 70); // Val 70%
            led_dsc.bg_color = mirror_col;
            lv_canvas_draw_rect(canvas, x, center_y + y, bar_w, SEGMENT_HEIGHT, &led_dsc);
        }

        // --- VẼ PEAK (VẠCH ĐỈNH) ---
        // Vẽ Peak dưới dạng một viên LED mỏng màu trắng
        if (peak_levels[i] > 0) {
            int peak_y = (int)peak_levels[i];
            
            // Snap to grid: Làm tròn vị trí Peak vào đúng ô lưới LED
            // Để Peak không bị lơ lửng giữa các khe hở
            int step = SEGMENT_HEIGHT + SEGMENT_GAP;
            peak_y = (peak_y / step) * step; 

            // Peak Trên
            lv_canvas_draw_rect(canvas, x, center_y - peak_y - 2, bar_w, 2, &peak_dsc);
            // Peak Dưới
            lv_canvas_draw_rect(canvas, x, center_y + peak_y, bar_w, 2, &peak_dsc);
        }
    }

    return MV_PAGE_RET_OK;
}

mv_page_t PeakMeterPage = {
    .sub_page_init = PeakMeter_sub_page_init,
    .sub_page_deinit = PeakMeter_sub_page_deinit,
    .sub_page_main_function = PeakMeter_sub_page_main_function,
    .state = MV_PAGE_INIT
};