#ifndef TACH_BOARD_H
#define TACH_BOARD_H

#include "esp_err.h"
#include <stdint.h>

/* All functions, including LVGL, run on the app task. */
esp_err_t board_init(void);
esp_err_t board_set_brightness(uint8_t percent);

#endif
