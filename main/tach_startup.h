#ifndef TACH_STARTUP_H
#define TACH_STARTUP_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#define TACH_STARTUP_DURATION_MS UINT64_C(10000)

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *progress;
    lv_obj_t *next_screen;
    uint64_t visible_since_ms;
    bool started;
    bool complete;
} tach_startup_t;

void tach_startup_create(tach_startup_t *startup, lv_obj_t *next_screen);
/* Call immediately after the first frame is visible, not during hardware init. */
void tach_startup_start(tach_startup_t *startup, uint64_t now_ms);
/* Returns true after loading next_screen; repeated calls are safe. */
bool tach_startup_update(tach_startup_t *startup, uint64_t now_ms);

#endif
