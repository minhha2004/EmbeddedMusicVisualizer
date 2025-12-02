// ==========================================
// FILE: arcreactor.c
// STYLE: Iron Man Arc Reactor + HUD Text
// ==========================================
#include "../music_visualizer_pages/mvpage.h" 
#include "arcreactor.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <graphic.h>

// Forward declaration
extern mv_page_t ArcReactorPage;

LV_IMG_DECLARE(back_icon_png);

// --- BIẾN TĨNH ---
static lv_obj_t *arc_cont = NULL;
static lv_obj_t *back_btn = NULL;

static lv_obj_t *arc_bass = NULL;
static lv_obj_t *arc_mid = NULL;
static lv_obj_t *arc_treble = NULL;

static lv_obj_t *core_glow = NULL;

// Biến lưu giá trị cũ
static int prev_bass = 0;
static int prev_mid = 0;
static int prev_treble = 0;

// --- HÀM BACK ---
static void back_event_handler(lv_event_t *e)
{
    (void)e;
    printf("ArcReactor: Back button clicked\n");

    extern mv_page_t *MusicVisualizerPage; 
    MusicVisualizerPage = NULL;

    lv_obj_t * screen = lv_scr_act(); 
    lv_obj_clean(screen);

    extern void mainpage_create(lv_obj_t *parent);
    mainpage_create(screen);
}

// --- HELPER: TẠO VÒNG CUNG ---
static lv_obj_t* create_reactor_ring(lv_obj_t *parent, int w, int h, lv_color_t color) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_arc_set_rotation(arc, 135);       
    lv_arc_set_bg_angles(arc, 0, 270);   
    lv_arc_set_value(arc, 0);            
    lv_obj_center(arc);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB); 
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    // Style nền (tối)
    lv_obj_set_style_arc_color(arc, lv_color_darken(color, 150), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 12, LV_PART_MAIN); // Mỏng hơn chút cho tinh tế

    // Style hiển thị (sáng)
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 12, LV_PART_INDICATOR);

    return arc;
}

// --- HELPER: TẠO CHỮ (LABEL) ---
static void create_hud_label(lv_obj_t *parent, const char* text, int y_offset, lv_color_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    
    // Căn chỉnh vào giữa theo chiều ngang, lệch theo chiều dọc
    lv_obj_align(label, LV_ALIGN_CENTER, 0, y_offset);
    
    // Màu chữ trùng màu vòng tròn
    lv_obj_set_style_text_color(label, color, 0);
    
    // Font chữ (Dùng font có sẵn của LVGL, bạn có thể đổi font to hơn nếu muốn)
    // Nếu font 14 bé quá thì thử 16 hoặc 20 nếu project đã enable
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0); 
}

// --- INIT ---
mv_page_err_code ArcReactor_sub_page_init(lv_obj_t *parent) {
    if (!parent) return MV_PAGE_RET_FAIL;
    ArcReactorPage.state = MV_PAGE_INIT;

    // 1. Nền
    arc_cont = lv_obj_create(parent);
    lv_obj_set_size(arc_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(arc_cont, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(arc_cont, LV_OBJ_FLAG_SCROLLABLE);

    // --- MÀU SẮC ---
    lv_color_t c_bass = lv_color_hex(0x0099FF);   // Xanh Dương
    lv_color_t c_mid = lv_color_hex(0x00FFCC);    // Xanh Ngọc
    lv_color_t c_treble = lv_color_hex(0xFFFFEE); // Trắng Vàng

    // 2. Tạo 3 vòng tròn
    // Kích thước: 350, 250, 150
    arc_bass = create_reactor_ring(arc_cont, 350, 350, c_bass);
    arc_mid = create_reactor_ring(arc_cont, 250, 250, c_mid);
    arc_treble = create_reactor_ring(arc_cont, 150, 150, c_treble);

    // 3. Tạo Chữ (Label) nằm ở khe hở phía dưới
    // Tính toán vị trí Y: (Bán kính) - 20px
    create_hud_label(arc_cont, "BASS", 160, c_bass);   // Dưới cùng
    create_hud_label(arc_cont, "MID", 110, c_mid);     // Giữa
    create_hud_label(arc_cont, "TREBLE", 60, c_treble);// Trên cùng (gần lõi)

    // 4. Lõi Core
    core_glow = lv_obj_create(arc_cont);
    lv_obj_set_size(core_glow, 60, 60); // Nhỏ lại chút để không che chữ Treble
    lv_obj_set_style_radius(core_glow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(core_glow, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(core_glow, LV_OPA_50, 0);
    
    // Hiệu ứng bóng
    lv_obj_set_style_shadow_width(core_glow, 40, 0);
    lv_obj_set_style_shadow_color(core_glow, lv_color_hex(0x00FFFF), 0);
    lv_obj_center(core_glow);

    // Nút Back
    back_btn = lv_btn_create(arc_cont);
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
mv_page_err_code ArcReactor_sub_page_deinit(void) {
    if (arc_cont) {
        lv_obj_del(arc_cont); // Xóa container cha là xóa hết con (arc, label, core)
        arc_cont = NULL;
        arc_bass = NULL;
        arc_mid = NULL;
        arc_treble = NULL;
        core_glow = NULL;
    }
    return MV_PAGE_RET_OK;
}

// --- UPDATE ---
mv_page_err_code ArcReactor_sub_page_main_function(mv_value_t *value) {
    if (!arc_cont || !value || !value->value) return MV_PAGE_RET_FAIL;

    float bass_sum = 0, mid_sum = 0, treble_sum = 0;
    
    // Chia dải tần
    for(int i=0; i<10; i++) bass_sum += fabsf(value->value[i]);
    bass_sum /= 10.0f;

    for(int i=10; i<60; i++) mid_sum += fabsf(value->value[i]);
    mid_sum /= 50.0f;

    for(int i=60; i<150; i++) treble_sum += fabsf(value->value[i]);
    treble_sum /= 90.0f;

    // Tính % (Sensitivity chỉnh ở đây)
    // Với tín hiệu ~60-80, hệ số 1.5 -> 1.8 là đẹp
    int target_bass = (int)(bass_sum * 1.5f);
    int target_mid = (int)(mid_sum * 1.8f);
    int target_treble = (int)(treble_sum * 3.0f);

    if(target_bass > 100) target_bass = 100;
    if(target_mid > 100) target_mid = 100;
    if(target_treble > 100) target_treble = 100;

    // Smoothing
    prev_bass = prev_bass * 0.7f + target_bass * 0.3f;
    prev_mid = prev_mid * 0.8f + target_mid * 0.2f;
    prev_treble = prev_treble * 0.5f + target_treble * 0.5f;

    // Update UI
    lv_arc_set_value(arc_bass, prev_bass);
    lv_arc_set_value(arc_mid, prev_mid);
    lv_arc_set_value(arc_treble, prev_treble);

    // Core Effect
    int opa_val = 100 + (prev_bass * 1.5);
    if (opa_val > 255) opa_val = 255;
    lv_obj_set_style_bg_opa(core_glow, (lv_opa_t)opa_val, 0);
    lv_obj_set_style_shadow_width(core_glow, 20 + (prev_bass / 2), 0);

    return MV_PAGE_RET_OK;
}

mv_page_t ArcReactorPage = {
    .sub_page_init = ArcReactor_sub_page_init,
    .sub_page_deinit = ArcReactor_sub_page_deinit,
    .sub_page_main_function = ArcReactor_sub_page_main_function,
    .state = MV_PAGE_INIT
};