// ==========================================
// FILE: circular.c (DEBUG VERSION)
// ==========================================
#include "circular.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// --- CẤU HÌNH DEBUG ---
// Đặt bằng 1 để bật chế độ GIẢ LẬP (Vòng tròn tự nhảy không cần nhạc)
// Đặt bằng 0 để dùng chế độ THẬT (Nghe nhạc từ MusicProcessor)
#define FAKE_MODE_ENABLE  0  

// 1. KHAI BÁO ẢNH 
LV_IMG_DECLARE(circle_bg_png);
LV_IMG_DECLARE(back_icon_png);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static lv_obj_t *circ_cont = NULL;
static lv_obj_t *circle_img = NULL;
static lv_obj_t *back_btn = NULL;
static float current_scale = 1.0f;

// --- SỰ KIỆN BACK ---
static void back_event_handler(lv_event_t *e)
{
    printf("Back button clicked\n");

    // 1. [QUAN TRỌNG] NGẮT KẾT NỐI VỚI THREAD XỬ LÝ NHẠC NGAY LẬP TỨC
    // Phải set biến toàn cục này về NULL để thread1 bên main.cpp không gọi hàm update nữa.
    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;

    // 2. Reset lại các con trỏ nội bộ để an toàn tuyệt đối
    // (Gọi hàm deinit để gán circle_img = NULL)
    Circular_sub_page_deinit();

    // 3. Bây giờ mới được phép xóa màn hình
    lv_obj_t * screen = lv_scr_act(); 
    lv_obj_clean(screen);

    // 4. Tạo lại Main Menu
    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(screen);
}

// --- INIT ---
mv_page_err_code Circular_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;

    CircularPage.state = MV_PAGE_INIT;

    circ_cont = lv_obj_create(parent);
    lv_obj_set_size(circ_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(circ_cont, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(circ_cont, LV_OBJ_FLAG_SCROLLABLE);

    circle_img = lv_img_create(circ_cont);
    lv_img_set_src(circle_img, &circle_bg_png);
    lv_obj_center(circle_img);
    // Pivot giữa ảnh
    lv_img_set_pivot(circle_img, circle_bg_png.header.w / 2, circle_bg_png.header.h / 2);

    back_btn = lv_btn_create(circ_cont);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(back_btn, back_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_icon = lv_img_create(back_btn);
    lv_img_set_src(back_icon, &back_icon_png);
    lv_obj_center(back_icon);

    return MV_PAGE_RET_OK;
}

// --- DEINIT ---
mv_page_err_code Circular_sub_page_deinit(void) {
    circ_cont = NULL;
    circle_img = NULL; 
    back_btn = NULL;
    return MV_PAGE_RET_OK;
}

// --- UPDATE (CÓ CHỨC NĂNG CHECK LỖI) ---
// --- UPDATE (ĐÃ CHỈNH LẠI THEO LOG THỰC TẾ) ---
mv_page_err_code Circular_sub_page_main_function(mv_value_t *value) {
    if (!circle_img || !value || !value->value) return MV_PAGE_RET_FAIL;

    // 1. Tính toán Bass
    float bass_energy = 0.0f;
    int samples = 30; 
    for(int i = 0; i < samples; i++) {
        float val = value->value[i];
        if (val < 0) val = -val;
        bass_energy += val;
    }
    bass_energy /= samples;

    // --- [SỬA Ở ĐÂY] ---
    // Log của bạn: 17 ~ 80.
    // Nếu dùng độ nhạy 40.0f cũ -> Zoom = 2000 (Quá lớn).
    // Ta cần Zoom khoảng 1.5 -> 2.0.
    // 60 * X = 0.8 => X = 0.013.
    
    // Giảm độ nhạy xuống 0.015f (tức là 1/1000 so với cũ)
    float sensitivity = 0.015f; 
    
    // Trừ bớt 10.0f (Noise floor) để khi nhạc nhỏ hẳn vòng tròn mới thu về
    float clean_bass = bass_energy - 10.0f;
    if (clean_bass < 0) clean_bass = 0;

    float target_scale = 1.0f + (clean_bass * sensitivity); 
    
    // --- GIỚI HẠN (CLAMP) ---
    // Mở rộng giới hạn lên 2.5 (250%) để nhảy bốc hơn vì tín hiệu bạn mạnh
    if (target_scale > 2.5f) target_scale = 2.5f;
    if (target_scale < 1.0f) target_scale = 1.0f;

    // --- LÀM MƯỢT (ATTACK/DECAY) ---
    if (target_scale > current_scale) {
        // Lên cực nhanh (Attack)
        current_scale = current_scale * 0.2f + target_scale * 0.8f; 
    } else {
        // Xuống từ từ (Decay) tạo độ nảy
        current_scale = current_scale * 0.85f + target_scale * 0.15f;
    }

    // Apply Zoom
    lv_img_set_zoom(circle_img, (int)(current_scale * 256));

    return MV_PAGE_RET_OK;
}

mv_page_t CircularPage = {
    .sub_page_init = Circular_sub_page_init,
    .sub_page_deinit = Circular_sub_page_deinit,
    .sub_page_main_function = Circular_sub_page_main_function,
    .state = MV_PAGE_INIT 
};