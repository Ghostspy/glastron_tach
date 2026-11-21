#ifndef TACH_UI_H
#define TACH_UI_H

#include "lvgl.h"
#include "tach_model.h"

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *arc;
    lv_obj_t *rpm;
    lv_obj_t *voltage;
    lv_obj_t *battery_caption;
    lv_obj_t *mode;
    lv_obj_t *theme_hint;
    bool night;
    char rpm_text[16];
    char voltage_text[16];
} tach_ui_t;

void tach_ui_create(tach_ui_t *ui);
void tach_ui_update(tach_ui_t *ui, const tach_view_t *view);
void tach_ui_set_night(tach_ui_t *ui, bool night);

#endif
