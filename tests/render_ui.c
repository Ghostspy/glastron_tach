#include "tach_ui.h"
#include "tach_startup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static lv_color_t framebuffer[480 * 480];
static lv_color_t draw_buffer[480 * 40];

static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels)
{
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) {
            if (x >= 0 && x < 480 && y >= 0 && y < 480) framebuffer[y * 480 + x] = *pixels;
            ++pixels;
        }
    }
    lv_disp_flush_ready(driver);
}

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "Usage: render-ui SCENE output.ppm\n"); return 2; }
    lv_init();
    static lv_disp_draw_buf_t buffer;
    static lv_disp_drv_t driver;
    lv_disp_draw_buf_init(&buffer, draw_buffer, NULL, 480 * 40);
    lv_disp_drv_init(&driver);
    driver.hor_res = 480;
    driver.ver_res = 480;
    driver.draw_buf = &buffer;
    driver.flush_cb = flush;
    assert(lv_disp_drv_register(&driver));
    tach_ui_t ui;
    tach_ui_create(&ui);
    tach_startup_t startup;
    const bool startup_scene = !strcmp(argv[1], "startup");
    if (startup_scene) {
        tach_startup_create(&startup, ui.screen);
        tach_startup_start(&startup, 0);
        assert(!tach_startup_update(&startup, 5000));
    }
    tach_model_t model = {.demo = true};
    tach_model_set_rpm(&model, 3250, 0);
    tach_model_set_voltage(&model, 13.8f, 0);
    if (!strcmp(argv[1], "low")) tach_model_set_voltage(&model, 11.6f, 0);
    else if (!strcmp(argv[1], "high")) tach_model_set_voltage(&model, 15.6f, 0);
    else if (!strcmp(argv[1], "missing")) model = (tach_model_t){0};
    else if (!strcmp(argv[1], "rpm-missing")) { model.rpm.valid = false; model.demo = false; }
    else if (!strcmp(argv[1], "voltage-missing")) { model.voltage.valid = false; model.demo = false; }
    else if (!strcmp(argv[1], "stopped")) tach_model_set_rpm(&model, 0, 0);
    else if (!strcmp(argv[1], "night")) tach_ui_set_night(&ui, true);
    else if (strcmp(argv[1], "normal") && !startup_scene) { fprintf(stderr, "Unknown scene\n"); return 2; }
    tach_view_t view = tach_model_view(&model, 0);
    tach_ui_update(&ui, &view);
    assert(!strcmp(lv_label_get_text(ui.rpm), view.rpm_valid ? view.rpm ? "3250" : "0" : "--"));
    assert(!strcmp(lv_label_get_text(ui.mode), view.demo ? "DEMO" :
        !view.rpm_valid && !view.voltage_valid ? "NO DATA" :
        !view.rpm_valid ? "RPM UNAVAILABLE" : "VOLTAGE UNAVAILABLE"));
    for (int i = 0; i < 5; ++i) { lv_tick_inc(40); lv_timer_handler(); }
    unsigned visible_rpm_pixels = 0;
    for (int y = 190; y < 270; ++y)
        for (int x = 160; x < 320; ++x)
            visible_rpm_pixels += framebuffer[y * 480 + x].full != 0;
    assert(visible_rpm_pixels > 200);

    /* Exercise transitions using the same persistent objects and text buffers. */
    tach_model_set_rpm(&model, 0, 500);
    tach_model_set_voltage(&model, 11.6f, 500);
    view = tach_model_view(&model, 500);
    tach_ui_update(&ui, &view);
    assert(!strcmp(lv_label_get_text(ui.rpm), "0"));
    assert(!strcmp(lv_label_get_text(ui.voltage), "11.6 V"));
    assert(!strcmp(lv_label_get_text(ui.battery_caption), "Low voltage"));
    tach_ui_update(&ui, &view); /* Repeated values may not corrupt static buffers. */
    assert(!strcmp(lv_label_get_text(ui.voltage), "11.6 V"));
    view = tach_model_view(&model, 2501);
    tach_ui_update(&ui, &view);
    assert(!strcmp(lv_label_get_text(ui.rpm), "--"));
    assert(!strcmp(lv_label_get_text(ui.voltage), "--.- V"));
    assert(lv_arc_get_value(ui.arc) == 0);
    tach_model_set_rpm(&model, 0, 2600);
    tach_model_set_voltage(&model, 13.8f, 2600);
    view = tach_model_view(&model, 2600);
    tach_ui_update(&ui, &view);
    assert(!strcmp(lv_label_get_text(ui.rpm), "0"));
    assert(!strcmp(lv_label_get_text(ui.battery_caption), "Battery"));

    /* The framebuffer above preserves the original scene for the output image. */
    FILE *out = fopen(argv[2], "wb");
    if (!out) { perror(argv[2]); return 1; }
    fprintf(out, "P6\n480 480\n255\n");
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 480; ++x) {
            lv_color32_t c = {.full = lv_color_to32(framebuffer[y * 480 + x])};
            /* Gray outside the physical round panel makes clipping visible. */
            if ((x - 240) * (x - 240) + (y - 240) * (y - 240) > 240 * 240) {
                c.ch.red = c.ch.green = c.ch.blue = 38;
            }
            const unsigned char rgb[] = {c.ch.red, c.ch.green, c.ch.blue};
            if (fwrite(rgb, 1, 3, out) != 3) return 1;
        }
    }
    return fclose(out) == 0 ? 0 : 1;
}
