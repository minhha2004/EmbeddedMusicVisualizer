// ==========================================
// FILE: waveform.c
// STYLE: Oscilloscope (Máy hiện sóng cổ điển)
// ==========================================
#include "../music_visualizer_pages/mvpage.h" 

#include "waveform.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <graphic.h>

// Forward declaration
extern mv_page_t WaveformPage;

LV_IMG_DECLARE(back_icon_png);

// Số điểm vẽ (Càng nhiều càng mượt, nhưng tốn CPU)
#define POINT_COUNT 128     
static int canvas_w = GRAPHIC_HOR_RES;
static int canvas_h = GRAPHIC_VER_RES; 

static lv_obj_t *wave_cont = NULL;
static lv_obj_t *canvas = NULL;
static lv_obj_t *back_btn = NULL;
static lv_color_t *cbuf = NULL;

// Mảng lưu giá trị cũ để làm mượt
static float prev_values[POINT_COUNT]; 

static void back_event_handler(lv_event_t *e)
{
    (void)e;
    printf("Waveform: Back button clicked\n");

    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;

    if (cbuf) { free(cbuf); cbuf = NULL; }
    
    lv_obj_t * screen = lv_scr_act(); 
    lv_obj_clean(screen);

    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(screen);
}

mv_page_err_code Waveform_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;

    WaveformPage.state = MV_PAGE_INIT;

    // 1. Container nền đen tuyền
    wave_cont = lv_obj_create(parent);
    lv_obj_set_size(wave_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(wave_cont, lv_color_hex(0x000000), 0); 
    lv_obj_clear_flag(wave_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Canvas
    canvas = lv_canvas_create(wave_cont);
    lv_obj_set_size(canvas, canvas_w, canvas_h);
    lv_obj_center(canvas);

    cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_w, canvas_h));
    if (cbuf == NULL) {
        printf("ERROR: Canvas Malloc Failed!\n");
        return MV_PAGE_RET_FAIL;
    }
    
    lv_canvas_set_buffer(canvas, cbuf, canvas_w, canvas_h, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    // Reset mảng làm mượt
    for(int i=0; i<POINT_COUNT; i++) prev_values[i] = 0.0f;

    // 3. Nút Back
    back_btn = lv_btn_create(wave_cont);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, back_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, &back_icon_png);
    lv_obj_center(back_icon);

    return MV_PAGE_RET_OK;
}

mv_page_err_code Waveform_sub_page_deinit(void) {
    if (canvas) { lv_obj_del(canvas); canvas = NULL; }
    if (wave_cont) { lv_obj_del(wave_cont); wave_cont = NULL; }
    if (cbuf) { free(cbuf); cbuf = NULL; }
    return MV_PAGE_RET_OK;
}

// --- HÀM VẼ LƯỚI (GRATICULE) ---
static void draw_oscilloscope_grid() {
    lv_draw_line_dsc_t grid_dsc;
    lv_draw_line_dsc_init(&grid_dsc);
    grid_dsc.width = 1;
    grid_dsc.color = lv_color_hex(0x333333); // Màu xám tối
    grid_dsc.opa = LV_OPA_50; // Mờ 50%

    // Vẽ 10 đường dọc
    int step_x = canvas_w / 10;
    for (int i = 1; i < 10; i++) {
        lv_point_t p1 = {i * step_x, 0};
        lv_point_t p2 = {i * step_x, canvas_h};
        lv_point_t points[2] = {p1, p2};
        lv_canvas_draw_line(canvas, points, 2, &grid_dsc);
    }

    // Vẽ 8 đường ngang
    int step_y = canvas_h / 8;
    for (int i = 1; i < 8; i++) {
        lv_point_t p1 = {0, i * step_y};
        lv_point_t p2 = {canvas_w, i * step_y};
        lv_point_t points[2] = {p1, p2};
        lv_canvas_draw_line(canvas, points, 2, &grid_dsc);
    }
    
    // Vẽ đường trục chính đậm hơn chút
    grid_dsc.color = lv_color_hex(0x555555);
    lv_point_t mid_p1 = {0, canvas_h / 2};
    lv_point_t mid_p2 = {canvas_w, canvas_h / 2};
    lv_point_t mid_points[2] = {mid_p1, mid_p2};
    lv_canvas_draw_line(canvas, mid_points, 2, &grid_dsc);
}

// --- UPDATE (OSCILLOSCOPE STYLE) ---
mv_page_err_code Waveform_sub_page_main_function(mv_value_t *value) {
    if (!canvas || !value || !value->value) return MV_PAGE_RET_FAIL;

    // 1. Xóa màn hình (Vẽ lại màu đen)
    lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    // 2. Vẽ lưới toạ độ (Để nhìn cho giống máy đo)
    draw_oscilloscope_grid();

    // 3. Cấu hình nét vẽ Sóng
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.width = 3; // Nét mảnh vừa phải (2-3px)
    line_dsc.color = lv_color_hex(0x00FF00); // MÀU XANH PHOSPHOR (Green Oscilloscope)
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;

    int center_y = canvas_h / 2;
    float step_x = (float)canvas_w / (float)(POINT_COUNT - 1); 

    // Biến lưu điểm trước đó để nối dây
    lv_point_t prev_point = {0, center_y};

    for (int i = 0; i < POINT_COUNT; i++) {
        // Lấy mẫu dữ liệu (nhảy cóc để dàn đều ra màn hình)
        int input_idx = i * (BAR_NUMBER / POINT_COUNT); 
        float raw_val = value->value[input_idx];
        if (raw_val < 0) raw_val = -raw_val;

        // --- LÀM MƯỢT ---
        prev_values[i] = prev_values[i] * 0.5f + raw_val * 0.5f;
        float val = prev_values[i];

        // --- TÍNH TOẠ ĐỘ Y ---
        // Hệ số 0.05f phù hợp với tín hiệu lớn (~60-80) của bạn
        int height = (int)(val * 0.05f * (canvas_h / 2)); 
        
        // Clamp
        if (height > (canvas_h / 2) - 5) height = (canvas_h / 2) - 5;

        // Oscilloscope hiển thị sóng Âm/Dương
        // Vì đây là FFT (chỉ có dương), ta vẽ dạng Line Spectrum
        int x = (int)(i * step_x);
        int y = center_y - height; // Sóng đi lên từ trục giữa

        lv_point_t current_point = {x, y};

        // --- VẼ ĐƯỜNG NỐI (CONNECT THE DOTS) ---
        if (i > 0) {
            lv_point_t line_points[2] = {prev_point, current_point};
            lv_canvas_draw_line(canvas, line_points, 2, &line_dsc);
            
            // [Hiệu ứng bóng mờ - Glow Effect]
            // Vẽ thêm 1 đường dọc mờ nhạt bên dưới để tạo cảm giác "đặc" hơn
            /* lv_draw_line_dsc_t glow_dsc = line_dsc;
            glow_dsc.width = 1;
            glow_dsc.opa = LV_OPA_30;
            lv_point_t p_ground = {x, center_y};
            lv_point_t p_glow[2] = {current_point, p_ground};
            lv_canvas_draw_line(canvas, p_glow, 2, &glow_dsc);
            */
        }

        // Lưu điểm hiện tại làm điểm cũ cho vòng lặp sau
        prev_point = current_point;
    }

    return MV_PAGE_RET_OK;
}

// --- STRUCT ---
mv_page_t WaveformPage = {
    .sub_page_init = Waveform_sub_page_init,
    .sub_page_deinit = Waveform_sub_page_deinit,
    .sub_page_main_function = Waveform_sub_page_main_function,
    .state = MV_PAGE_INIT
};