// ==========================================
// FILE: peakmeter.c
// STYLE: Mirrored Peak Meter (Có hạt rơi trọng lực)
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
#define BAR_COUNT 64       // Số lượng cột (Ít hơn Waveform để cột to và rõ)
#define PEAK_GRAVITY 1.5f  // Tốc độ rơi của hạt đỉnh
#define PEAK_HOLD_TIME 10  // Thời gian giữ đỉnh (số khung hình) trước khi rơi

static int canvas_w = GRAPHIC_HOR_RES;
static int canvas_h = GRAPHIC_VER_RES; 

static lv_obj_t *peak_cont = NULL;
static lv_obj_t *canvas = NULL;
static lv_obj_t *back_btn = NULL;
static lv_color_t *cbuf = NULL;

// Mảng lưu chiều cao hiện tại (để làm mượt)
static float bar_heights[BAR_COUNT];
// Mảng lưu vị trí của hạt đỉnh (Peak)
static float peak_levels[BAR_COUNT];
// Mảng đếm ngược thời gian giữ đỉnh
static int peak_hold_timers[BAR_COUNT];

// --- HÀM BACK ---
static void back_event_handler(lv_event_t *e) {
    (void)e;
    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;
    if (cbuf) { free(cbuf); cbuf = NULL; }
    lv_obj_clean(lv_scr_act());
    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(lv_scr_act());
}

// --- INIT ---
mv_page_err_code PeakMeter_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;
    PeakMeterPage.state = MV_PAGE_INIT;

    peak_cont = lv_obj_create(parent);
    lv_obj_set_size(peak_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(peak_cont, lv_color_hex(0x050505), 0); // Đen sẫm
    lv_obj_clear_flag(peak_cont, LV_OBJ_FLAG_SCROLLABLE);

    canvas = lv_canvas_create(peak_cont);
    lv_obj_set_size(canvas, canvas_w, canvas_h);
    lv_obj_center(canvas);

    cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_w, canvas_h));
    if (!cbuf) return MV_PAGE_RET_FAIL;
    
    lv_canvas_set_buffer(canvas, cbuf, canvas_w, canvas_h, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x050505), LV_OPA_COVER);

    // Reset dữ liệu
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

// --- UPDATE ---
mv_page_err_code PeakMeter_sub_page_main_function(mv_value_t *value) {
    if (!canvas || !value || !value->value) return MV_PAGE_RET_FAIL;

    // Xóa nền
    lv_canvas_fill_bg(canvas, lv_color_hex(0x050505), LV_OPA_COVER);

    lv_draw_rect_dsc_t bar_dsc;
    lv_draw_rect_dsc_init(&bar_dsc);
    
    // Config cho hạt Peak (Màu trắng sáng)
    lv_draw_rect_dsc_t peak_dsc;
    lv_draw_rect_dsc_init(&peak_dsc);
    peak_dsc.bg_color = lv_color_hex(0xFFFFFF); 

    int center_y = canvas_h / 2;
    float bar_width_float = (float)canvas_w / (float)BAR_COUNT;
    int bar_w = (int)bar_width_float - 4; // Trừ 4px để tạo khe hở giữa các cột
    if (bar_w < 1) bar_w = 1;

    int max_h = (canvas_h / 2) - 10; // Chiều cao tối đa

    for (int i = 0; i < BAR_COUNT; i++) {
        // Lấy dữ liệu
        int input_idx = i * (BAR_NUMBER / BAR_COUNT); 
        float raw_val = value->value[input_idx];
        if (raw_val < 0) raw_val = -raw_val;

        // 1. LÀM MƯỢT CỘT SÓNG
        // Lên nhanh (0.4), xuống chậm (0.8)
        if (raw_val > bar_heights[i]) {
            bar_heights[i] = bar_heights[i] * 0.6f + raw_val * 0.4f;
        } else {
            bar_heights[i] = bar_heights[i] * 0.85f + raw_val * 0.15f;
        }

        // 2. TÍNH CHIỀU CAO HIỂN THỊ
        // Tín hiệu to (60-80) -> nhân 0.04 là đẹp
        int h = (int)(bar_heights[i] * 0.04f * (float)max_h);
        if (h > max_h) h = max_h;
        if (h < 0) h = 0;

        // 3. XỬ LÝ VẬT LÝ CHO HẠT PEAK (Trọng tâm của style này)
        if (h >= peak_levels[i]) {
            // Nếu cột sóng cao hơn hạt -> Đẩy hạt lên ngay lập tức
            peak_levels[i] = (float)h;
            peak_hold_timers[i] = PEAK_HOLD_TIME; // Reset thời gian giữ
        } else {
            // Nếu cột sóng thấp hơn hạt
            if (peak_hold_timers[i] > 0) {
                // Đang trong thời gian giữ đỉnh -> Hạt đứng yên trên không trung
                peak_hold_timers[i]--;
            } else {
                // Hết thời gian giữ -> Hạt bắt đầu rơi tự do
                peak_levels[i] -= PEAK_GRAVITY;
                // Nếu rơi xuống thấp hơn cột sóng thì dừng lại ở đầu cột sóng
                if (peak_levels[i] < h) peak_levels[i] = (float)h;
            }
        }

        // 4. VẼ
        int x = (int)(i * bar_width_float) + 2;
        
        // -- Màu Gradient theo chiều ngang (Cyan -> Purple -> Red) --
        int hue = 160 - (i * 160 / BAR_COUNT); // 160 (Xanh) về 0 (Đỏ)
        if (hue < 0) hue += 360;
        bar_dsc.bg_color = lv_color_hsv_to_rgb(hue, 90, 100);

        // -- Vẽ Cột Trên --
        lv_canvas_draw_rect(canvas, x, center_y - h, bar_w, h, &bar_dsc);
        // -- Vẽ Cột Dưới --
        lv_canvas_draw_rect(canvas, x, center_y, bar_w, h, &bar_dsc);

        // -- Vẽ Hạt Peak --
        int peak_y_top = center_y - (int)peak_levels[i] - 2; // Cách đỉnh 2px
        int peak_y_bot = center_y + (int)peak_levels[i];
        
        if (peak_levels[i] > 2) { // Chỉ vẽ nếu có độ cao
            // Hạt trên
            lv_canvas_draw_rect(canvas, x, peak_y_top, bar_w, 2, &peak_dsc);
            // Hạt dưới
            lv_canvas_draw_rect(canvas, x, peak_y_bot, bar_w, 2, &peak_dsc);
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