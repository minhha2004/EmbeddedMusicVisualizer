// ==========================================
// FILE: mainpage.c
// STYLE: Modern Dark UI (Card Layout) - Fixed for LVGL v8
// ==========================================
#include "mainpage.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../music_visualizer_pages/mvpage.h"
#include <graphic.h>

mainpage_t g_mainpage;

// Biến đếm ID
static uint32_t item_id_counter = 0;

// Đường dẫn ảnh
static const char* MENU_IMG_PATH = "/home/asus/EmbeddedMusicVisualizer(Visual)/Graphic/images_src/";

/********* STATIC PROTOTYPES *********/
static void container_click_event_cb(lv_event_t *e);
static void mainpage_add_card(const char *title, const char *subtitle, const char *img_name);

/********************************************
 * MAINPAGE CREATE  
 ********************************************/
void mainpage_create(lv_obj_t *parent)
{
    // Reset ID để tránh lỗi nhảy trang
    item_id_counter = 0; 

    memset(&g_mainpage, 0, sizeof(mainpage_t));

    // 1. TẠO CONTAINER CHÍNH (Nền tối)
    g_mainpage.main_container = lv_obj_create(parent);
    lv_obj_set_size(g_mainpage.main_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_mainpage.main_container, lv_color_hex(0x1a1a1a), 0); // Xám đen đậm
    lv_obj_clear_flag(g_mainpage.main_container, LV_OBJ_FLAG_SCROLLABLE);

    // 2. TẠO TIÊU ĐỀ (HEADER)
    lv_obj_t * header = lv_obj_create(g_mainpage.main_container);
    lv_obj_set_size(header, LV_PCT(100), 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_50, 0); // Mờ 50%
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(header, 2, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x00dac6), 0); // Viền xanh Neon

    lv_obj_t * title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, "MUSIC VISUALIZER");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_20, 0); // Font to (nếu có)
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_letter_space(title_lbl, 2, 0); // Giãn chữ
    lv_obj_center(title_lbl);


    // 3. TẠO DANH SÁCH CUỘN (SCROLL LIST)
    g_mainpage.subpage_list = lv_obj_create(g_mainpage.main_container);
    lv_obj_set_size(g_mainpage.subpage_list, LV_PCT(100), LV_PCT(85));
    lv_obj_align(g_mainpage.subpage_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(g_mainpage.subpage_list, LV_OPA_TRANSP, 0); // Trong suốt
    lv_obj_set_style_border_width(g_mainpage.subpage_list, 0, 0);
    
    // Sử dụng Flex Layout để tự sắp xếp các thẻ
    lv_obj_set_layout(g_mainpage.subpage_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_mainpage.subpage_list, LV_FLEX_FLOW_COLUMN); // Xếp dọc
    lv_obj_set_flex_align(g_mainpage.subpage_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(g_mainpage.subpage_list, 20, 0); // Padding xung quanh
    lv_obj_set_style_pad_row(g_mainpage.subpage_list, 15, 0); // Khoảng cách giữa các thẻ

    /*** ADD VISUALIZER CARDS ***/
    
    // ID 0
    mainpage_add_card(
        "Basic Spectrum",
        "Classic bar analyzer.",
        "basic_visual_image.png"
    );

    // ID 1
    mainpage_add_card(
        "Circular Neon",
        "Pulsing circle beat.",
        "circular.png"
    );

    // ID 2
    mainpage_add_card(
        "Waveform Scope",
        "Oscilloscope style line.",
        "waveform.png"
    );

    // ID 3
    mainpage_add_card(
        "Arc Reactor",
        "Iron Man HUD interface.",
        "circular.png" 
    );

    // ID 4
    mainpage_add_card(
        "Particle Fountain",
        "Firework effects.",
        "circular.png" 
    );

    // ID 5
    mainpage_add_card(
        "Peak Meter Mirror",
        "Gravity peaks mirror.",
        "basic_visual_image.png" 
    );

    // ID 6: Pink Diamond 3D
    mainpage_add_card(
        "Pink Diamonds",
        "Floating jewels reacting to bass.",
        "basic_visual_image.png" 
    );
}

/********************************************
 * EVENT: CLICK ITEM
 ********************************************/
static void container_click_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    uint32_t *item_id = lv_obj_get_user_data(obj);

    if (!item_id) return;

    printf("=== OPENING APP ID: %u ===\n", *item_id);

    if (SetSubpage(*item_id) == MV_PAGE_RET_OK) {
        if (MusicVisualizerPage && MusicVisualizerPage->sub_page_init) {
            MusicVisualizerPage->sub_page_init(g_mainpage.main_container);
        }
    } else {
        printf("ERROR: App ID %u not found!\n", *item_id);
    }
}

/********************************************
 * HELPER: ADD CARD (THẺ GIAO DIỆN ĐẸP)
 ********************************************/
// TRONG FILE: Graphic/main_page/mainpage.c

static void mainpage_add_card(const char *title, const char *subtitle, const char *img_name)
{
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "%s%s", MENU_IMG_PATH, img_name);

    // 1. Tạo Card Container
    lv_obj_t *card = lv_obj_create(g_mainpage.subpage_list);
    lv_obj_set_size(card, LV_PCT(90), 110); // [SỬA] Tăng chiều cao thẻ lên 110px để thoáng hơn
    lv_obj_set_style_radius(card, 15, 0);
    
    // Gradient Background
    lv_obj_set_style_bg_color(card, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_HOR, 0);
    
    // Viền & Bóng
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x555555), 0);
    lv_obj_set_style_shadow_width(card, 20, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_60, 0);

    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Style Pressed
    lv_obj_set_style_bg_color(card, lv_color_hex(0x00dac6), LV_STATE_PRESSED);
    lv_obj_set_style_transform_width(card, -5, LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(card, -5, LV_STATE_PRESSED);

    uint32_t *id = malloc(sizeof(uint32_t));
    *id = item_id_counter++;
    lv_obj_set_user_data(card, id);
    lv_obj_add_event_cb(card, container_click_event_cb, LV_EVENT_CLICKED, NULL);

    // 2. Icon
    lv_obj_t *img = lv_img_create(card);
    lv_img_set_src(img, fullpath);
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 15, 0);
    lv_img_set_zoom(img, 180);

    // 3. Title (TIÊU ĐỀ) - ĐÃ SỬA
    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    
    // [SỬA 1] Tăng cỡ chữ lên 20 (Nếu lỗi biên dịch hãy đổi về 16 hoặc bật font trong lv_conf.h)
    // Nếu bạn muốn ĐẬM, hãy dùng font Bold (ví dụ &lv_font_montserrat_20) nếu đã enable.
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0); 
    
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    
    // [SỬA 2] Thêm khoảng cách giữa các ký tự (giả lập độ đậm và thoáng)
    lv_obj_set_style_text_letter_space(lbl_title, 1, 0); 

    // [SỬA 3] Đẩy vị trí lên cao hơn một chút (Y = 12)
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 90, 15); 

    // 4. Subtitle (MÔ TẢ) - ĐÃ SỬA
    lv_obj_t *lbl_sub = lv_label_create(card);
    lv_label_set_text(lbl_sub, subtitle);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0xAAAAAA), 0);
    
    // [SỬA 4] Đẩy vị trí xuống thấp hơn (Y = -15) để tách xa tiêu đề
    lv_obj_align(lbl_sub, LV_ALIGN_BOTTOM_LEFT, 90, -15);
    
    // 5. Mũi tên
    lv_obj_t *arrow = lv_label_create(card);
    lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(arrow, lv_color_hex(0x00dac6), 0);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -15, 0);
}