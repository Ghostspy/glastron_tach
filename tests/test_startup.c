#include "tach_startup.h"
#include <assert.h>
#include <stdio.h>

static lv_color_t pixels[480 * 10];

static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *data)
{
    (void)area;
    (void)data;
    lv_disp_flush_ready(driver);
}

static void check_startup(uint64_t shown_at)
{
    lv_obj_t *gauge = lv_obj_create(NULL);
    lv_scr_load(gauge);
    tach_startup_t startup;
    tach_startup_create(&startup, gauge);
    assert(lv_scr_act() == startup.screen);
    assert(lv_scr_act() != gauge);
    assert(!tach_startup_update(&startup, shown_at + 20000));
    tach_startup_start(&startup, shown_at);
    assert(!tach_startup_update(&startup, shown_at));
    assert(lv_bar_get_value(startup.progress) == 0);
    assert(!tach_startup_update(&startup, shown_at + 5000));
    assert(lv_bar_get_value(startup.progress) == 500);
    assert(!tach_startup_update(&startup, shown_at + 9999));
    assert(lv_scr_act() != gauge);
    assert(tach_startup_update(&startup, shown_at + 10000));
    assert(lv_scr_act() == gauge);
    assert(startup.screen == NULL && startup.progress == NULL);
    assert(tach_startup_update(&startup, shown_at + 10001));
    assert(lv_scr_act() == gauge);
}

int main(void)
{
    lv_init();
    static lv_disp_draw_buf_t buffer;
    static lv_disp_drv_t driver;
    lv_disp_draw_buf_init(&buffer, pixels, NULL, 480 * 10);
    lv_disp_drv_init(&driver);
    driver.hor_res = driver.ver_res = 480;
    driver.draw_buf = &buffer;
    driver.flush_cb = flush;
    assert(lv_disp_drv_register(&driver));
    check_startup(12345);
    check_startup(UINT64_C(0xffffffff) - 3000);
    puts("PASS: visible-start timing, 9999/10000 ms transition, and long uptime");
    return 0;
}
