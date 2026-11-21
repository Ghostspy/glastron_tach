#include "tach_ui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define FACE 0x000000
#define WHITE 0xF5F6F2
#define MUTED 0x819099
#define BLUE 0x65C7F0
#define AMBER 0xFFC15A
#define TRACK 0x26333D

LV_FONT_DECLARE(tach_font_rpm);

/* All point arrays outlive LVGL line objects, which retain their pointers. */
static lv_point_t ticks[36][2];
static const char *const scale_numbers[] = {"0", "1", "2", "3", "4", "5", "6", "7"};
static const lv_point_t divider[] = {{170, 311}, {310, 311}};
static const lv_point_t battery_outline[] = {
    {160, 344}, {181, 344}, {181, 349}, {185, 349},
    {185, 359}, {181, 359}, {181, 364}, {160, 364}, {160, 344}
};

static void set_text(lv_obj_t *obj, const char *text)
{
    if (strcmp(lv_label_get_text(obj), text)) lv_label_set_text_static(obj, text);
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                       uint32_t color, int x, int y, int width)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(obj, width);
    lv_obj_set_pos(obj, x, y);
    lv_label_set_text_static(obj, text);
    return obj;
}

static void line(lv_obj_t *parent, const lv_point_t *points, int count,
                 int width, uint32_t color)
{
    lv_obj_t *obj = lv_line_create(parent);
    lv_line_set_points(obj, points, (uint16_t)count);
    lv_obj_set_style_line_width(obj, width, 0);
    lv_obj_set_style_line_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_line_rounded(obj, true, 0);
}

void tach_ui_create(tach_ui_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(ui->screen);
    lv_obj_set_size(ui->screen, 480, 480);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(FACE), 0);
    lv_obj_set_style_bg_opa(ui->screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);

    ui->arc = lv_arc_create(ui->screen);
    lv_obj_remove_style_all(ui->arc);
    lv_obj_set_size(ui->arc, 454, 454);
    lv_obj_set_pos(ui->arc, 13, 13);
    lv_arc_set_rotation(ui->arc, 135);
    lv_arc_set_bg_angles(ui->arc, 0, 270);
    lv_arc_set_range(ui->arc, 0, TACH_SCALE_MAX_RPM);
    lv_obj_set_style_arc_width(ui->arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui->arc, lv_color_hex(TRACK), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ui->arc, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui->arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ui->arc, lv_color_hex(BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(ui->arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ui->arc, true, LV_PART_INDICATOR);
    lv_obj_clear_flag(ui->arc, LV_OBJ_FLAG_CLICKABLE);

    for (int i = 0; i <= 35; ++i) {
        const float a = (135.0f + 270.0f * (float)i / 35.0f) * 0.01745329252f;
        const bool major = i % 5 == 0;
        const float r = major ? 194.0f : 204.0f;
        ticks[i][0] = (lv_point_t){(lv_coord_t)lroundf(240 + r * cosf(a)),
                                  (lv_coord_t)lroundf(240 + r * sinf(a))};
        ticks[i][1] = (lv_point_t){(lv_coord_t)lroundf(240 + 214 * cosf(a)),
                                  (lv_coord_t)lroundf(240 + 214 * sinf(a))};
        line(ui->screen, ticks[i], 2, major ? 3 : 1, major ? WHITE : MUTED);
        if (major) {
            const int x = (int)lroundf(240 + 173 * cosf(a)) - 18;
            const int y = (int)lroundf(240 + 173 * sinf(a)) - 14;
            label(ui->screen, scale_numbers[i / 5], &lv_font_montserrat_24,
                  WHITE, x, y, 36);
        }
    }

    label(ui->screen, "MERCURY 115", &lv_font_montserrat_18, WHITE, 130, 129, 220);
    label(ui->screen, "RPM x 1000", &lv_font_montserrat_14, MUTED, 170, 158, 140);
    ui->rpm = label(ui->screen, "--", &tach_font_rpm, WHITE, 120, 187, 240);
    label(ui->screen, "RPM", &lv_font_montserrat_18, MUTED, 190, 278, 100);
    line(ui->screen, divider, 2, 1, TRACK);
    line(ui->screen, battery_outline, 9, 2, MUTED);
    ui->voltage = label(ui->screen, "--.- V", &lv_font_montserrat_32,
                        WHITE, 190, 337, 134);
    ui->battery_caption = label(ui->screen, "Battery", &lv_font_montserrat_16,
                                 MUTED, 135, 377, 210);
    ui->mode = label(ui->screen, "NO DATA", &lv_font_montserrat_14, AMBER, 140, 412, 200);
    ui->theme_hint = label(ui->screen, "", &lv_font_montserrat_14, MUTED, 175, 438, 130);
    lv_scr_load(ui->screen);
}

void tach_ui_set_night(tach_ui_t *ui, bool night)
{
    ui->night = night;
    lv_obj_set_style_arc_color(ui->arc, lv_color_hex(night ? AMBER : BLUE), LV_PART_INDICATOR);
    lv_label_set_text_static(ui->theme_hint, night ? "Night" : "");
}

void tach_ui_update(tach_ui_t *ui, const tach_view_t *view)
{
    char text[16];
    if (view->rpm_valid) snprintf(text, sizeof text, "%d", view->rpm);
    else snprintf(text, sizeof text, "--");
    if (strcmp(lv_label_get_text(ui->rpm), text)) {
        strcpy(ui->rpm_text, text);
        /* Label uses an owned buffer; static API avoids per-update allocation. */
        lv_label_set_text_static(ui->rpm, ui->rpm_text);
    }
    if (view->voltage_valid) snprintf(text, sizeof text, "%.1f V", (double)view->voltage);
    else snprintf(text, sizeof text, "--.- V");
    if (strcmp(lv_label_get_text(ui->voltage), text)) {
        strcpy(ui->voltage_text, text);
        lv_label_set_text_static(ui->voltage, ui->voltage_text);
    }
    lv_arc_set_value(ui->arc, view->rpm_valid ? view->arc_rpm : 0);
    lv_obj_set_style_text_color(ui->rpm, lv_color_hex(view->rpm_valid ? WHITE : MUTED), 0);

    const bool warning = view->voltage_status == TACH_VOLTAGE_LOW ||
                         view->voltage_status == TACH_VOLTAGE_HIGH;
    lv_obj_set_style_text_color(ui->voltage, lv_color_hex(warning ? AMBER :
                                view->voltage_valid ? WHITE : MUTED), 0);
    lv_obj_set_style_text_color(ui->battery_caption, lv_color_hex(warning ? AMBER : MUTED), 0);
    set_text(ui->battery_caption,
        view->voltage_status == TACH_VOLTAGE_LOW ? "Low voltage" :
        view->voltage_status == TACH_VOLTAGE_HIGH ? "High voltage" :
        view->voltage_valid ? "Battery" : "Voltage unavailable");
    set_text(ui->mode, view->demo ? "DEMO" :
        !view->rpm_valid && !view->voltage_valid ? "NO DATA" :
        !view->rpm_valid ? "RPM UNAVAILABLE" : !view->voltage_valid ? "VOLTAGE UNAVAILABLE" : "");
}
