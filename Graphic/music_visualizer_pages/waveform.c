// ==========================================
// FILE: waveform.c
// STYLE: Oscilloscope (Classic Green Phosphor CRT Style)
// ==========================================
#include "../music_visualizer_pages/mvpage.h" 

#include "waveform.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <graphic.h>

extern mv_page_t WaveformPage;

LV_IMG_DECLARE(back_icon_png);

// --- CẤU HÌNH ---
#define POINT_COUNT 64      
static int canvas_w = GRAPHIC_HOR_RES;
static int canvas_h = GRAPHIC_VER_RES; 

// [MÀU SẮC MỚI]
// Màu nền: Xanh rêu tối (Deep dark phosphor green)
#define CRT_BG_COLOR lv_color_hex(0x031503) 
// Màu lưới: Xanh lá mờ
#define CRT_GRID_COLOR lv_color_hex(0x00AA00)
// Màu sóng: Xanh neon sáng rực
#define CRT_TRACE_COLOR lv_color_hex(0x00FF00)


static lv_obj_t *wave_cont = NULL;
static lv_obj_t *canvas = NULL;
static lv_obj_t *back_btn = NULL;
static lv_color_t *cbuf = NULL;

static float prev_values[POINT_COUNT]; 
static float smooth_data[POINT_COUNT];

static void back_event_handler(lv_event_t *e)
{
    (void)e;
    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;
    if (cbuf) { free(cbuf); cbuf = NULL; }
    lv_obj_clean(lv_scr_act());
    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(lv_scr_act());
}

mv_page_err_code Waveform_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;
    WaveformPage.state = MV_PAGE_INIT;

    wave_cont = lv_obj_create(parent);
    lv_obj_set_size(wave_cont, LV_PCT(100), LV_PCT(100));
    // [SỬA MÀU NỀN CONTAINER]
    lv_obj_set_style_bg_color(wave_cont, CRT_BG_COLOR, 0); 
    lv_obj_clear_flag(wave_cont, LV_OBJ_FLAG_SCROLLABLE);

    canvas = lv_canvas_create(wave_cont);
    lv_obj_set_size(canvas, canvas_w, canvas_h);
    lv_obj_center(canvas);

    cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_w, canvas_h));
    if (cbuf == NULL) return MV_PAGE_RET_FAIL;
    
    lv_canvas_set_buffer(canvas, cbuf, canvas_w, canvas_h, LV_IMG_CF_TRUE_COLOR);
    // [SỬA MÀU NỀN CANVAS]
    lv_canvas_fill_bg(canvas, CRT_BG_COLOR, LV_OPA_COVER);

    for(int i=0; i<POINT_COUNT; i++) {
        prev_values[i] = 0.0f;
        smooth_data[i] = 0.0f;
    }

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

static void draw_oscilloscope_grid() {
    lv_draw_line_dsc_t grid_dsc;
    lv_draw_line_dsc_init(&grid_dsc);
    grid_dsc.width = 1;
    // [SỬA MÀU LƯỚI]
    grid_dsc.color = CRT_GRID_COLOR; 
    grid_dsc.opa = LV_OPA_30; // Rất mờ

    int step_x = canvas_w / 10;
    for (int i = 1; i < 10; i++) {
        lv_point_t p1 = {i * step_x, 0};
        lv_point_t p2 = {i * step_x, canvas_h};
        lv_point_t points[2] = {p1, p2};
        lv_canvas_draw_line(canvas, points, 2, &grid_dsc);
    }
    int step_y = canvas_h / 8;
    for (int i = 1; i < 8; i++) {
        lv_point_t p1 = {0, i * step_y};
        lv_point_t p2 = {canvas_w, i * step_y};
        lv_point_t points[2] = {p1, p2};
        lv_canvas_draw_line(canvas, points, 2, &grid_dsc);
    }
    // Trục giữa (Sáng hơn một chút)
    grid_dsc.color = CRT_TRACE_COLOR; // Dùng màu sáng hơn cho trục
    grid_dsc.opa = LV_OPA_50;
    lv_point_t m1 = {0, canvas_h/2};
    lv_point_t m2 = {canvas_w, canvas_h/2};
    lv_point_t mp[2] = {m1, m2};
    lv_canvas_draw_line(canvas, mp, 2, &grid_dsc);
}

// --- THUẬT TOÁN CATMULL-ROM SPLINE ---
static float catmull_rom(float p0, float p1, float p2, float p3, float t) {
    float v0 = (p2 - p0) * 0.5f;
    float v1 = (p3 - p1) * 0.5f;
    float t2 = t * t;
    float t3 = t * t2;
    return (2 * p1 - 2 * p2 + v0 + v1) * t3 + (-3 * p1 + 3 * p2 - 2 * v0 - v1) * t2 + v0 * t + p1;
}

// --- UPDATE ---
mv_page_err_code Waveform_sub_page_main_function(mv_value_t *value) {
    if (!canvas || !value || !value->value) return MV_PAGE_RET_FAIL;

    // 1. Xóa màn hình (Dùng màu nền CRT mới)
    lv_canvas_fill_bg(canvas, CRT_BG_COLOR, LV_OPA_COVER);
    draw_oscilloscope_grid();

    int center_y = canvas_h / 2;
    
    // --- BƯỚC 1: LẤY DỮ LIỆU & LÀM MƯỢT ---
    for (int i = 0; i < POINT_COUNT; i++) {
        int idx = i * (BAR_NUMBER / POINT_COUNT);
        float raw = value->value[idx];
        if (raw < 0) raw = -raw;

        prev_values[i] = prev_values[i] * 0.5f + raw * 0.5f;
        
        smooth_data[i] = prev_values[i] * 0.05f * (canvas_h / 2);
        
        if (smooth_data[i] > (canvas_h/2 - 5)) smooth_data[i] = canvas_h/2 - 5;
    }

    for (int k=0; k<2; k++) {
        for (int i = 1; i < POINT_COUNT - 1; i++) {
            smooth_data[i] = (smooth_data[i-1] + smooth_data[i] + smooth_data[i+1]) / 3.0f;
        }
    }

    // --- BƯỚC 2: VẼ ĐƯỜNG CONG (SPLINE DRAWING) ---
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    // [SỬA ĐỘ DÀY VÀ MÀU SẮC SÓNG]
    line_dsc.width = 4; // Tăng độ dày lên 4 để trông "phát sáng" hơn
    line_dsc.color = CRT_TRACE_COLOR; // Xanh neon rực rỡ
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;

    lv_point_t prev_point = {0, center_y - (int)smooth_data[0]};
    
    int draw_step = 2; 

    for (int x = draw_step; x < canvas_w; x += draw_step) {
        
        float map_idx = (float)x / (float)canvas_w * (POINT_COUNT - 1);
        
        int i = (int)map_idx; 
        float t = map_idx - i; 

        float p0 = (i > 0) ? smooth_data[i - 1] : smooth_data[0];
        float p1 = smooth_data[i];
        float p2 = (i < POINT_COUNT - 1) ? smooth_data[i + 1] : smooth_data[POINT_COUNT - 1];
        float p3 = (i < POINT_COUNT - 2) ? smooth_data[i + 2] : p2;

        float spline_y = catmull_rom(p0, p1, p2, p3, t);
        
        int y = center_y - (int)spline_y;
        lv_point_t current_point = {x, y};

        lv_point_t line_points[2] = {prev_point, current_point};
        lv_canvas_draw_line(canvas, line_points, 2, &line_dsc);

        prev_point = current_point;
    }

    return MV_PAGE_RET_OK;
}

mv_page_t WaveformPage = {
    .sub_page_init = Waveform_sub_page_init,
    .sub_page_deinit = Waveform_sub_page_deinit,
    .sub_page_main_function = Waveform_sub_page_main_function,
    .state = MV_PAGE_INIT
};