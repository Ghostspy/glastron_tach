#include "board.h"
#include "tach_model.h"
#include "tach_ui.h"
#include "tach_startup.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "tach";
static tach_ui_t ui;
static tach_model_t model;
static tach_startup_t startup;

static uint64_t milliseconds(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static void toggle_night(lv_event_t *event)
{
    (void)event;
    tach_ui_set_night(&ui, !ui.night);
    ESP_ERROR_CHECK(board_set_brightness(ui.night ? CONFIG_TACH_NIGHT_BRIGHTNESS :
                                                   CONFIG_TACH_DAY_BRIGHTNESS));
}

void app_main(void)
{
#if CONFIG_TACH_DEMO_MODE
    const bool demo = true;
#else
    const bool demo = false;
#endif
    ESP_LOGI(TAG, "Boat tach v0.1.1; reset reason=%d; demo=%s",
             (int)esp_reset_reason(), demo ? "yes" : "no");
    ESP_LOGW(TAG, "Display prototype: no engine or boat voltage inputs configured");
    ESP_ERROR_CHECK(board_init());
    tach_ui_create(&ui);
    lv_obj_add_flag(ui.screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui.screen, toggle_night, LV_EVENT_CLICKED, NULL);
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    tach_startup_create(&startup, ui.screen);
    lv_refr_now(NULL);
    ESP_ERROR_CHECK(board_set_brightness(CONFIG_TACH_DAY_BRIGHTNESS));
    /* Count ten seconds of visible logo time, excluding hardware initialization. */
    const uint64_t started = milliseconds();
    tach_startup_start(&startup, started);
    uint64_t last_tick = started;
    uint64_t last_sample = started;
    uint64_t gauge_started = started;
    bool first_sample = true;
    bool gauge_visible = false;
    for (;;) {
        const uint64_t now = milliseconds();
        lv_tick_inc((uint32_t)(now - last_tick));
        last_tick = now;
        if (!gauge_visible && tach_startup_update(&startup, now)) {
            gauge_visible = true;
            gauge_started = now;
        }
        if (gauge_visible && (first_sample || now - last_sample >= 40u)) {
#if CONFIG_TACH_DEMO_MODE
            tach_demo_sample(&model, now - gauge_started, now);
#else
            (void)gauge_started;
#endif
            /* Future sensor provider updates model here, on this same task.
             * With demo off, readings intentionally remain unavailable. */
            const tach_view_t view = tach_model_view(&model, now);
            tach_ui_update(&ui, &view);
            last_sample = now;
            first_sample = false;
        }
        lv_timer_handler();
        ESP_ERROR_CHECK(esp_task_wdt_reset());
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
