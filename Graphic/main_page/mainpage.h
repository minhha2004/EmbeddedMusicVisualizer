#ifndef MAINPAGE_H
#define MAINPAGE_H

#include "../lvgl/lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SUBPAGES 10

typedef struct {
    lv_obj_t *main_container;      
    lv_obj_t *subpage_list;        
} mainpage_t;

extern mainpage_t g_mainpage;

void mainpage_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* MAINPAGE_H */
