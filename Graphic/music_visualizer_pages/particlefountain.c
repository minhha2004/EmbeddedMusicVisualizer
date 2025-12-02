// ==========================================
// FILE: particlefountain.c
// STYLE: Beat Detection (Chỉ phun khi có nhịp)
// ==========================================
#include "../music_visualizer_pages/mvpage.h" 
#include "particlefountain.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <graphic.h>

extern mv_page_t ParticleFountainPage;
LV_IMG_DECLARE(back_icon_png);

// --- CẤU HÌNH ---
#define PARTICLE_COUNT 200
#define GRAVITY 0.7f        // Trọng lực mạnh hơn để rơi dứt khoát
#define FRICTION 0.9f 
#define BOUNCE_FACTOR 0.6f 

static int canvas_w = GRAPHIC_HOR_RES;
static int canvas_h = GRAPHIC_VER_RES; 

typedef struct {
    float x, y;
    float vx, vy;
    int life;
    int max_life;
    lv_color_t color;
    int size;
} Particle;

static lv_obj_t *part_cont = NULL;
static lv_obj_t *canvas = NULL;
static lv_obj_t *back_btn = NULL;
static lv_color_t *cbuf = NULL;
static Particle particles[PARTICLE_COUNT]; 

// BIẾN LƯU MỨC NĂNG LƯỢNG TRUNG BÌNH (ĐỂ SO SÁNH)
static float average_energy = 0.0f; 

static float random_float(float min, float max) {
    return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

static lv_color_t get_fire_color(float life_percent) {
    if (life_percent > 0.8f) return lv_color_hex(0xFFFFFF); 
    if (life_percent > 0.6f) return lv_color_hex(0xFFFF00); 
    if (life_percent > 0.3f) return lv_color_hex(0xFF5500); 
    return lv_color_hex(0xAA0000);                          
}

static void back_event_handler(lv_event_t *e) {
    (void)e;
    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;
    if (cbuf) { free(cbuf); cbuf = NULL; }
    lv_obj_clean(lv_scr_act());
    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(lv_scr_act());
}

mv_page_err_code Particle_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;
    ParticleFountainPage.state = MV_PAGE_INIT;

    part_cont = lv_obj_create(parent);
    lv_obj_set_size(part_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(part_cont, lv_color_hex(0x101010), 0);
    lv_obj_clear_flag(part_cont, LV_OBJ_FLAG_SCROLLABLE);

    canvas = lv_canvas_create(part_cont);
    lv_obj_set_size(canvas, canvas_w, canvas_h);
    lv_obj_center(canvas);

    cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_w, canvas_h));
    if (!cbuf) return MV_PAGE_RET_FAIL;
    
    lv_canvas_set_buffer(canvas, cbuf, canvas_w, canvas_h, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x101010), LV_OPA_COVER);

    for(int i=0; i<PARTICLE_COUNT; i++) particles[i].life = 0;
    
    // Reset mức trung bình
    average_energy = 0.0f;

    back_btn = lv_btn_create(part_cont);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, back_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, &back_icon_png);
    lv_obj_center(back_icon);

    return MV_PAGE_RET_OK;
}

mv_page_err_code Particle_sub_page_deinit(void) {
    if (canvas) { lv_obj_del(canvas); canvas = NULL; }
    if (part_cont) { lv_obj_del(part_cont); part_cont = NULL; }
    if (cbuf) { free(cbuf); cbuf = NULL; }
    return MV_PAGE_RET_OK;
}

static void spawn_particle(float power) {
    for(int i=0; i<PARTICLE_COUNT; i++) {
        if (particles[i].life <= 0) {
            particles[i].x = canvas_w / 2;
            particles[i].y = canvas_h - 5; 

            // Spread (Tản ra 2 bên)
            float spread = 15.0f; 
            particles[i].vx = random_float(-spread, spread);

            // Power (Bay cao)
            float launch_power = 12.0f + (power * 0.3f); 
            particles[i].vy = -random_float(launch_power * 0.5f, launch_power);

            particles[i].life = (int)random_float(30, 60);
            particles[i].max_life = particles[i].life;
            particles[i].size = (int)random_float(3, 6);
            return; 
        }
    }
}

mv_page_err_code Particle_sub_page_main_function(mv_value_t *value) {
    if (!canvas || !value || !value->value) return MV_PAGE_RET_FAIL;

    lv_canvas_fill_bg(canvas, lv_color_hex(0x101010), LV_OPA_COVER);

    // 1. Tính Bass Hiện Tại (Instant Energy)
    float instant_energy = 0;
    int samples = 15;
    for(int i=0; i<samples; i++) {
        float val = value->value[i];
        if (val < 0) val = -val;
        instant_energy += val;
    }
    instant_energy /= samples;

    // 2. THUẬT TOÁN BEAT DETECTION (QUAN TRỌNG)
    // Tính mức độ đột biến: (Năng lượng hiện tại) - (Trung bình * Hệ số)
    // Hệ số 1.3 nghĩa là phải to hơn mức trung bình 30% mới coi là Beat
    
    // Nếu chưa khởi tạo trung bình
    if (average_energy == 0) average_energy = instant_energy;

    // Tính độ chênh lệch
    float diff = instant_energy - (average_energy * 1.1f); // Ngưỡng nhạy: 1.1

    // Điều kiện kích hoạt:
    // 1. Phải to hơn trung bình (diff > 0)
    // 2. Độ chênh lệch phải đủ lớn (> 10.0 đơn vị) để tránh nhiễu
    // 3. Bản thân âm thanh phải đủ to (> 30.0)
    if (diff > 10.0f && instant_energy > 30.0f) {
        
        // Càng đột biến mạnh -> Phun càng nhiều
        // diff thường khoảng 10-40. 
        int spawn_count = (int)(diff / 4.0f);
        
        if (spawn_count > 15) spawn_count = 15; // Max 1 lúc
        
        for(int k=0; k<spawn_count; k++) {
            spawn_particle(diff); // Truyền độ chênh lệch vào để tính độ cao
        }
    }

    // 3. Cập nhật mức trung bình (Moving Average)
    // Học theo nhạc: 90% giữ cái cũ, 10% nạp cái mới
    // Giúp "average" bám đuổi theo "instant" một cách từ từ
    average_energy = average_energy * 0.9f + instant_energy * 0.1f;


    // 4. Cập nhật & Vẽ
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);

    for(int i=0; i<PARTICLE_COUNT; i++) {
        if (particles[i].life > 0) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy += GRAVITY; 

            // Nảy
            if (particles[i].y >= canvas_h - 2) {
                particles[i].y = canvas_h - 2; 
                particles[i].vy = -particles[i].vy * BOUNCE_FACTOR;
                particles[i].vx *= FRICTION;
                if (fabs(particles[i].vy) < 1.0f) particles[i].vy = 0;
            }
            // Tường
            if (particles[i].x <= 0 || particles[i].x >= canvas_w) {
                particles[i].vx = -particles[i].vx * 0.8f; 
                if (particles[i].x <= 0) particles[i].x = 1;
                if (particles[i].x >= canvas_w) particles[i].x = canvas_w - 1;
            }

            particles[i].life--;

            float life_pct = (float)particles[i].life / (float)particles[i].max_life;
            rect_dsc.bg_color = get_fire_color(life_pct);

            lv_canvas_draw_rect(canvas, (int)particles[i].x, (int)particles[i].y, 
                              particles[i].size, particles[i].size, &rect_dsc);
        }
    }

    return MV_PAGE_RET_OK;
}

mv_page_t ParticleFountainPage = {
    .sub_page_init = Particle_sub_page_init,
    .sub_page_deinit = Particle_sub_page_deinit,
    .sub_page_main_function = Particle_sub_page_main_function,
    .state = MV_PAGE_INIT
};