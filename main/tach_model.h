#ifndef TACH_MODEL_H
#define TACH_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#define TACH_SCALE_MAX_RPM 7000
#define TACH_STALE_MS 2000u

typedef struct {
    float value;
    uint64_t updated_ms;
    bool valid;
} tach_reading_t;

typedef struct {
    tach_reading_t rpm;
    tach_reading_t voltage;
    bool demo;
} tach_model_t;

typedef enum {
    TACH_VOLTAGE_UNKNOWN,
    TACH_VOLTAGE_NORMAL,
    TACH_VOLTAGE_LOW,
    TACH_VOLTAGE_HIGH
} tach_voltage_status_t;

typedef struct {
    bool rpm_valid;
    bool voltage_valid;
    bool demo;
    int rpm;
    int arc_rpm;
    float voltage;
    tach_voltage_status_t voltage_status;
} tach_view_t;

void tach_model_set_rpm(tach_model_t *model, float rpm, uint64_t now_ms);
void tach_model_set_voltage(tach_model_t *model, float volts, uint64_t now_ms);
tach_view_t tach_model_view(const tach_model_t *model, uint64_t now_ms);
void tach_demo_sample(tach_model_t *model, uint64_t elapsed_ms, uint64_t now_ms);

#endif
