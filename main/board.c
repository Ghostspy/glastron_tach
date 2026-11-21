#include "board.h"
#include "lvgl.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define TRY(call) do { esp_err_t result_ = (call); if (result_ != ESP_OK) return result_; } while (0)
#define EXPANDER 0x20
#define LCD_RESET (1u << 0)
#define TOUCH_RESET (1u << 1)
#define LCD_CS (1u << 2)
#define I2C_TIMEOUT_MS 25
#define DRAW_LINES 30

static const char *TAG = "board";
static uint8_t outputs = 0x0f; /* Resets released, LCD/SD CS high, buzzer low. */
static esp_lcd_panel_handle_t panel;
static spi_device_handle_t panel_spi;
static uint8_t touch_address;
static lv_disp_draw_buf_t draw_buffer;
static lv_disp_drv_t display_driver;
static lv_indev_drv_t touch_driver;

typedef struct {
    uint8_t command;
    uint8_t length;
    uint8_t data[16];
    uint16_t delay_ms;
} panel_command_t;
#include "panel_init.h"

static esp_err_t i2c_write8(uint8_t address, uint8_t reg, uint8_t value)
{
    const uint8_t bytes[] = {reg, value};
    return i2c_master_write_to_device(I2C_NUM_0, address, bytes, sizeof bytes,
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static esp_err_t expander_update(uint8_t mask, bool high)
{
    const uint8_t next = high ? outputs | mask : outputs & (uint8_t)~mask;
    TRY(i2c_write8(EXPANDER, 1, next));
    outputs = next;
    return ESP_OK;
}

static esp_err_t touch_read16(uint8_t address, uint16_t reg, uint8_t *data, size_t size)
{
    const uint8_t prefix[] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    return i2c_master_write_read_device(I2C_NUM_0, address, prefix, 2,
        data, size, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static esp_err_t touch_clear(void)
{
    const uint8_t clear[] = {0x81, 0x4e, 0};
    return i2c_master_write_to_device(I2C_NUM_0, touch_address, clear,
                                      sizeof clear, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static void read_touch(lv_indev_drv_t *driver, lv_indev_data_t *data)
{
    (void)driver;
    static lv_point_t last_point;
    static bool pressed;
    data->point = last_point;
    data->state = LV_INDEV_STATE_REL;
    if (!touch_address) return;
    uint8_t status;
    if (touch_read16(touch_address, 0x814e, &status, 1) != ESP_OK) {
        pressed = false;
        return;
    }
    if (!(status & 0x80)) {
        data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        return;
    }
    const unsigned count = status & 0x0f;
    pressed = false;
    if (count > 0 && count <= 5) {
        uint8_t point[8];
        if (touch_read16(touch_address, 0x814f, point, sizeof point) == ESP_OK) {
            const unsigned x = point[1] | ((unsigned)point[2] << 8);
            const unsigned y = point[3] | ((unsigned)point[4] << 8);
            if (x < 480 && y < 480) {
                last_point = (lv_point_t){(lv_coord_t)x, (lv_coord_t)y};
                pressed = true;
            }
        }
    }
    if (touch_clear() != ESP_OK) pressed = false;
    data->point = last_point;
    data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

static esp_err_t panel_byte(bool is_data, uint8_t value)
{
    spi_transaction_t transaction = {
        .cmd = is_data ? 1 : 0,
        .addr = value
    };
    return spi_device_polling_transmit(panel_spi, &transaction);
}

static esp_err_t panel_initialize(void)
{
    const spi_bus_config_t bus = {
        .mosi_io_num = 1, .miso_io_num = -1, .sclk_io_num = 2,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    TRY(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    const spi_device_interface_config_t device = {
        .command_bits = 1, .address_bits = 8, .mode = 0,
        .clock_speed_hz = 4000000, .spics_io_num = -1, .queue_size = 1
    };
    TRY(spi_bus_add_device(SPI2_HOST, &device, &panel_spi));
    TRY(expander_update(LCD_CS, false));
    for (size_t i = 0; i < sizeof panel_commands / sizeof panel_commands[0]; ++i) {
        TRY(panel_byte(false, panel_commands[i].command));
        for (size_t j = 0; j < panel_commands[i].length; ++j)
            TRY(panel_byte(true, panel_commands[i].data[j]));
        if (panel_commands[i].delay_ms)
            vTaskDelay(pdMS_TO_TICKS(panel_commands[i].delay_ms));
    }
    TRY(expander_update(LCD_CS, true));
    TRY(spi_bus_remove_device(panel_spi));
    TRY(spi_bus_free(SPI2_HOST));

    const esp_lcd_rgb_panel_config_t config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = 16,
        .num_fbs = 1,
        .psram_trans_align = 64,
        .bounce_buffer_size_px = 480 * 10,
        .hsync_gpio_num = 38, .vsync_gpio_num = 39,
        .de_gpio_num = 40, .pclk_gpio_num = 41, .disp_gpio_num = -1,
        .data_gpio_nums = {5, 45, 48, 47, 21, 14, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17},
        .timings = {
            .pclk_hz = 18000000, .h_res = 480, .v_res = 480,
            .hsync_back_porch = 10, .hsync_front_porch = 50, .hsync_pulse_width = 8,
            .vsync_back_porch = 18, .vsync_front_porch = 8, .vsync_pulse_width = 2,
            .flags.pclk_active_neg = false
        },
        .flags.fb_in_psram = true
    };
    TRY(esp_lcd_new_rgb_panel(&config, &panel));
    TRY(esp_lcd_panel_reset(panel));
    TRY(esp_lcd_panel_init(panel));
    return ESP_OK;
}

static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels)
{
    /* RGB driver synchronously copies this partial draw buffer into its PSRAM FB. */
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1,
        area->x2 + 1, area->y2 + 1, pixels));
    lv_disp_flush_ready(driver);
}

esp_err_t board_set_brightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    TRY(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8191u * percent / 100u));
    TRY(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    return ESP_OK;
}

esp_err_t board_init(void)
{
    /* Hold the backlight dark throughout initialization. */
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK
    };
    TRY(ledc_timer_config(&timer));
    const ledc_channel_config_t backlight = {
        .gpio_num = 6, .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0, .duty = 0
    };
    TRY(ledc_channel_config(&backlight));

    const i2c_config_t i2c = {
        .mode = I2C_MODE_MASTER, .sda_io_num = 15, .scl_io_num = 7,
        .sda_pullup_en = true, .scl_pullup_en = true, .master.clk_speed = 400000
    };
    TRY(i2c_param_config(I2C_NUM_0, &i2c));
    TRY(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
    TRY(i2c_write8(EXPANDER, 1, outputs));
    /* IMU/RTC interrupt lines remain inputs; resets/CS/buzzer are outputs. */
    TRY(i2c_write8(EXPANDER, 3, 0x70));
    TRY(expander_update(LCD_RESET | TOUCH_RESET, false));
    const gpio_config_t touch_int = {
        .pin_bit_mask = 1ULL << 16, .mode = GPIO_MODE_OUTPUT
    };
    TRY(gpio_config(&touch_int));
    TRY(gpio_set_level(16, 0));
    vTaskDelay(pdMS_TO_TICKS(20));
    TRY(expander_update(LCD_RESET | TOUCH_RESET, true));
    vTaskDelay(pdMS_TO_TICKS(10));
    TRY(gpio_set_direction(16, GPIO_MODE_INPUT));
    vTaskDelay(pdMS_TO_TICKS(100));
    TRY(panel_initialize());

    uint8_t id[4] = {0};
    const uint8_t addresses[] = {0x5d, 0x14};
    for (size_t i = 0; i < sizeof addresses; ++i) {
        if (touch_read16(addresses[i], 0x8140, id, sizeof id) == ESP_OK &&
            id[0] == '9' && id[1] == '1' && id[2] == '1') {
            touch_address = addresses[i];
            ESP_LOGI(TAG, "GT911 touch at 0x%02x", touch_address);
            break;
        }
    }
    if (!touch_address) ESP_LOGW(TAG, "Touch unavailable; gauge remains operational");

    lv_init();
    lv_color_t *buffer = heap_caps_malloc(480 * DRAW_LINES * sizeof(lv_color_t),
                                         MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buffer) return ESP_ERR_NO_MEM;
    lv_disp_draw_buf_init(&draw_buffer, buffer, NULL, 480 * DRAW_LINES);
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = 480;
    display_driver.ver_res = 480;
    display_driver.draw_buf = &draw_buffer;
    display_driver.flush_cb = flush;
    lv_disp_t *display = lv_disp_drv_register(&display_driver);
    if (!display) return ESP_ERR_NO_MEM;
    if (touch_address) {
        lv_indev_drv_init(&touch_driver);
        touch_driver.type = LV_INDEV_TYPE_POINTER;
        touch_driver.disp = display;
        touch_driver.read_cb = read_touch;
        if (!lv_indev_drv_register(&touch_driver)) return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
