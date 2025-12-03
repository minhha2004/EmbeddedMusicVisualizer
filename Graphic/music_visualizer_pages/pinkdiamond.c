// ==========================================
// FILE: pinkdiamond.c
// STYLE: 3D Objects + Collision (Va chạm vật lý)
// ==========================================
#include "../music_visualizer_pages/mvpage.h" 
#include "pinkdiamond.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <graphic.h>

extern mv_page_t PinkDiamondPage;

LV_IMG_DECLARE(back_icon_png);
LV_IMG_DECLARE(pink_diamond_png); 

// --- CẤU HÌNH ---
#define DIAMOND_COUNT 12     // Giảm nhẹ số lượng để va chạm đỡ bị kẹt
#define FOV 400.0f           
#define BOUNDS_X 1600        // Không gian rộng
#define BOUNDS_Y 900        
#define Z_MIN 50.0f          
#define Z_MAX 900.0f         

// Bán kính va chạm (giả sử trong không gian 3D, kim cương to khoảng 150 đơn vị)
#define COLLISION_RADIUS 180.0f 

static int screen_w = GRAPHIC_HOR_RES;
static int screen_h = GRAPHIC_VER_RES; 

typedef struct {
    lv_obj_t *img_obj;
    float x, y, z;     
    float vx, vy, vz;  
    // Đã bỏ các biến rotation
} Diamond3D;

static lv_obj_t *cont = NULL;
static lv_obj_t *back_btn = NULL;
static Diamond3D diamonds[DIAMOND_COUNT];

static float random_float(float min, float max) {
    return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

static void back_event_handler(lv_event_t *e) {
    (void)e;
    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;
    lv_obj_clean(lv_scr_act());
    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(lv_scr_act());
}

static void init_diamond(int i) {
    diamonds[i].x = random_float(-BOUNDS_X, BOUNDS_X);
    diamonds[i].y = random_float(-BOUNDS_Y, BOUNDS_Y);
    diamonds[i].z = random_float(Z_MIN, Z_MAX);

    // Tốc độ bay vừa phải
    diamonds[i].vx = random_float(-1.5f, 1.5f); 
    diamonds[i].vy = random_float(-1.5f, 1.5f);
    diamonds[i].vz = random_float(-0.5f, 0.5f); 

    diamonds[i].img_obj = lv_img_create(cont);
    lv_img_set_src(diamonds[i].img_obj, &pink_diamond_png);
    lv_img_set_pivot(diamonds[i].img_obj, pink_diamond_png.header.w / 2, pink_diamond_png.header.h / 2);
    
    // [QUAN TRỌNG] Reset góc xoay về 0 (Thẳng đứng)
    lv_img_set_angle(diamonds[i].img_obj, 0);
}

// --- HÀM XỬ LÝ VA CHẠM GIỮA CÁC VIÊN KIM CƯƠNG ---
static void resolve_collisions() {
    for (int i = 0; i < DIAMOND_COUNT; i++) {
        for (int j = i + 1; j < DIAMOND_COUNT; j++) {
            
            // Tính khoảng cách giữa 2 viên (dx, dy, dz)
            float dx = diamonds[j].x - diamonds[i].x;
            float dy = diamonds[j].y - diamonds[i].y;
            float dz = diamonds[j].z - diamonds[i].z;
            
            // Khoảng cách bình phương (để đỡ phải căn bậc 2 nếu chưa cần)
            float distSq = dx*dx + dy*dy + dz*dz;
            
            // Khoảng cách tối thiểu để không chạm nhau (Radius 1 + Radius 2)
            float minDist = COLLISION_RADIUS * 2; 

            // Nếu khoảng cách thực tế < khoảng cách tối thiểu -> Đang chạm nhau
            if (distSq < minDist * minDist) {
                float dist = sqrtf(distSq);
                if (dist < 0.1f) dist = 0.1f; // Tránh chia cho 0

                // 1. Phản hồi đàn hồi (Đổi hướng vận tốc)
                // Đơn giản hóa: Trao đổi vận tốc cho nhau (Elastic Collision approximation)
                // (Cách này tạo ra hiệu ứng nảy hỗn loạn rất vui mắt)
                float temp_vx = diamonds[i].vx;
                float temp_vy = diamonds[i].vy;
                float temp_vz = diamonds[i].vz;

                diamonds[i].vx = diamonds[j].vx;
                diamonds[i].vy = diamonds[j].vy;
                diamonds[i].vz = diamonds[j].vz;

                diamonds[j].vx = temp_vx;
                diamonds[j].vy = temp_vy;
                diamonds[j].vz = temp_vz;

                // 2. Chống dính (Overlap Correction)
                // Đẩy 2 viên ra xa nhau một chút để không bị kẹt dính vào nhau
                float overlap = minDist - dist;
                float nx = dx / dist; // Vector hướng
                float ny = dy / dist;
                float nz = dz / dist;

                // Đẩy mỗi viên ra một nửa khoảng chồng lấn
                float push = overlap * 0.5f;
                
                diamonds[i].x -= nx * push;
                diamonds[i].y -= ny * push;
                diamonds[i].z -= nz * push;
                
                diamonds[j].x += nx * push;
                diamonds[j].y += ny * push;
                diamonds[j].z += nz * push;
            }
        }
    }
}

mv_page_err_code PinkDiamond_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;
    PinkDiamondPage.state = MV_PAGE_INIT;

    cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x1a0033), 0); 
    lv_obj_set_style_bg_grad_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_dir(cont, LV_GRAD_DIR_VER, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    for(int i=0; i<DIAMOND_COUNT; i++) {
        init_diamond(i);
    }

    back_btn = lv_btn_create(cont);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, back_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, &back_icon_png);
    lv_obj_center(back_icon);

    return MV_PAGE_RET_OK;
}

mv_page_err_code PinkDiamond_sub_page_deinit(void) {
    if (cont) { lv_obj_del(cont); cont = NULL; }
    return MV_PAGE_RET_OK;
}

// --- UPDATE ---
mv_page_err_code PinkDiamond_sub_page_main_function(mv_value_t *value) {
    if (!cont || !value || !value->value) return MV_PAGE_RET_FAIL;

    // Tính Bass
    float bass_sum = 0;
    for(int i=0; i<20; i++) bass_sum += fabsf(value->value[i]);
    bass_sum /= 20.0f;

    float music_speed = 1.0f + (bass_sum * 0.015f); 
    float music_scale = 1.0f + (bass_sum * 0.01f);

    int center_x = screen_w / 2;
    int center_y = screen_h / 2;

    // --- BƯỚC 1: XỬ LÝ DI CHUYỂN & TƯỜNG ---
    for(int i=0; i<DIAMOND_COUNT; i++) {
        // Cập nhật vị trí
        diamonds[i].x += diamonds[i].vx * music_speed;
        diamonds[i].y += diamonds[i].vy * music_speed;
        diamonds[i].z += diamonds[i].vz * music_speed;

        // Xử lý nảy tường (Bounce Wall)
        if (diamonds[i].x > BOUNDS_X) { diamonds[i].x = BOUNDS_X; diamonds[i].vx *= -1; }
        else if (diamonds[i].x < -BOUNDS_X) { diamonds[i].x = -BOUNDS_X; diamonds[i].vx *= -1; }

        if (diamonds[i].y > BOUNDS_Y) { diamonds[i].y = BOUNDS_Y; diamonds[i].vy *= -1; }
        else if (diamonds[i].y < -BOUNDS_Y) { diamonds[i].y = -BOUNDS_Y; diamonds[i].vy *= -1; }

        if (diamonds[i].z > Z_MAX) { diamonds[i].z = Z_MAX; diamonds[i].vz *= -1; }
        else if (diamonds[i].z < Z_MIN) { diamonds[i].z = Z_MIN; diamonds[i].vz *= -1; }
        
        // Luôn đảm bảo góc là 0 (Thẳng đứng)
        lv_img_set_angle(diamonds[i].img_obj, 0);
    }

    // --- BƯỚC 2: XỬ LÝ VA CHẠM GIỮA CÁC VIÊN (MỚI) ---
    resolve_collisions();

    // --- BƯỚC 3: HIỂN THỊ LÊN MÀN HÌNH ---
    for(int i=0; i<DIAMOND_COUNT; i++) {
        float scale_factor = FOV / (FOV + diamonds[i].z);
        
        int x_2d = center_x + (int)(diamonds[i].x * scale_factor);
        int y_2d = center_y + (int)(diamonds[i].y * scale_factor);

        lv_obj_set_pos(diamonds[i].img_obj, x_2d, y_2d);

        int final_zoom = (int)(200.0f * scale_factor * music_scale);
        if (final_zoom < 10) final_zoom = 10;
        if (final_zoom > 2000) final_zoom = 2000;

        lv_img_set_zoom(diamonds[i].img_obj, final_zoom);
        
        int opacity = (int)(255 * scale_factor);
        if (opacity > 255) opacity = 255;
        if (opacity < 60) opacity = 60;
        lv_obj_set_style_img_opa(diamonds[i].img_obj, opacity, 0);
    }

    return MV_PAGE_RET_OK;
}

mv_page_t PinkDiamondPage = {
    .sub_page_init = PinkDiamond_sub_page_init,
    .sub_page_deinit = PinkDiamond_sub_page_deinit,
    .sub_page_main_function = PinkDiamond_sub_page_main_function,
    .state = MV_PAGE_INIT
};