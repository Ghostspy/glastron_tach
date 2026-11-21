#include "tach_startup.h"
#include <string.h>

LV_IMG_DECLARE(glastron_logo);

void tach_startup_create(tach_startup_t *startup, lv_obj_t *next_screen)
{
    memset(startup, 0, sizeof(*startup));
    startup->next_screen = next_screen;
    startup->screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(startup->screen);
    lv_obj_set_size(startup->screen, 480, 480);
    lv_obj_set_style_bg_color(startup->screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(startup->screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(startup->screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *logo = lv_img_create(startup->screen);
    lv_img_set_src(logo, &glastron_logo);
    lv_obj_set_style_img_recolor(logo, lv_color_hex(0xF5F6F2), 0);
    lv_obj_set_style_img_recolor_opa(logo, LV_OPA_COVER, 0);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, -26);

    lv_obj_t *model = lv_label_create(startup->screen);
    lv_obj_set_style_text_font(model, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(model, lv_color_hex(0x819099), 0);
    lv_label_set_text_static(model, "SSV-175");
    lv_obj_align(model, LV_ALIGN_TOP_MID, 0, 292);

    startup->progress = lv_bar_create(startup->screen);
    lv_obj_remove_style_all(startup->progress);
    lv_obj_set_size(startup->progress, 200, 4);
    lv_obj_set_pos(startup->progress, 140, 334);
    lv_obj_set_style_bg_color(startup->progress, lv_color_hex(0x26333D), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(startup->progress, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(startup->progress, lv_color_hex(0x65C7F0), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(startup->progress, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(startup->progress, 0, 1000);
    lv_bar_set_value(startup->progress, 0, LV_ANIM_OFF);

    lv_obj_t *loading = lv_label_create(startup->screen);
    lv_obj_set_style_text_font(loading, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(loading, lv_color_hex(0x819099), 0);
    lv_label_set_text_static(loading, "Loading...");
    lv_obj_align(loading, LV_ALIGN_TOP_MID, 0, 355);
    lv_scr_load(startup->screen);
}

void tach_startup_start(tach_startup_t *startup, uint64_t now_ms)
{
    if (!startup->started) {
        startup->visible_since_ms = now_ms;
        startup->started = true;
    }
}

bool tach_startup_update(tach_startup_t *startup, uint64_t now_ms)
{
    if (startup->complete) return true;
    if (!startup->started || now_ms < startup->visible_since_ms) return false;
    const uint64_t elapsed = now_ms - startup->visible_since_ms;
    if (elapsed >= TACH_STARTUP_DURATION_MS) {
        lv_scr_load(startup->next_screen);
        lv_obj_del(startup->screen);
        startup->screen = NULL;
        startup->progress = NULL;
        startup->complete = true;
        return true;
    }
    lv_bar_set_value(startup->progress,
        (int32_t)(elapsed * 1000u / TACH_STARTUP_DURATION_MS), LV_ANIM_OFF);
    return false;
}
