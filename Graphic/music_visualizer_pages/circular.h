#ifndef CIRCULAR_H
#define CIRCULAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl/lvgl.h>
#include "../music_visualizer_pages/mvpage.h" 

// Khai báo prototype hàm
mv_page_err_code Circular_sub_page_init(lv_obj_t *parent);
mv_page_err_code Circular_sub_page_deinit(void);
mv_page_err_code Circular_sub_page_main_function(mv_value_t *value);

// Khai báo biến struct để mvpage.c nhìn thấy
extern mv_page_t CircularPage; 

#ifdef __cplusplus
}
#endif

#endif