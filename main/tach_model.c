#include "tach_model.h"
#include <math.h>

/* Presentation bands only: tune after measuring this charging system. */
#ifndef TACH_VOLTAGE_LOW_V
#define TACH_VOLTAGE_LOW_V 12.0f
#endif
#ifndef TACH_VOLTAGE_HIGH_V
#define TACH_VOLTAGE_HIGH_V 15.0f
#endif

void tach_model_set_rpm(tach_model_t *model, float rpm, uint64_t now_ms)
{
    model->rpm = (tach_reading_t){rpm, now_ms,
        isfinite(rpm) && rpm >= 0.0f && rpm <= 15000.0f};
}

void tach_model_set_voltage(tach_model_t *model, float volts, uint64_t now_ms)
{
    model->voltage = (tach_reading_t){volts, now_ms,
        isfinite(volts) && volts >= 1.0f && volts <= 36.0f};
}

static bool fresh(tach_reading_t reading, uint64_t now_ms)
{
    return reading.valid && now_ms >= reading.updated_ms &&
           now_ms - reading.updated_ms <= TACH_STALE_MS;
}

tach_view_t tach_model_view(const tach_model_t *model, uint64_t now_ms)
{
    tach_view_t view = {
        .rpm_valid = fresh(model->rpm, now_ms),
        .voltage_valid = fresh(model->voltage, now_ms),
        .demo = model->demo,
        .voltage_status = TACH_VOLTAGE_UNKNOWN
    };
    if (view.rpm_valid) {
        view.rpm = (int)(model->rpm.value + 0.5f);
        view.arc_rpm = view.rpm > TACH_SCALE_MAX_RPM ? TACH_SCALE_MAX_RPM : view.rpm;
    }
    if (view.voltage_valid) {
        view.voltage = model->voltage.value;
        view.voltage_status = view.voltage < TACH_VOLTAGE_LOW_V ? TACH_VOLTAGE_LOW :
            view.voltage > TACH_VOLTAGE_HIGH_V ? TACH_VOLTAGE_HIGH : TACH_VOLTAGE_NORMAL;
    }
    return view;
}

void tach_demo_sample(tach_model_t *model, uint64_t elapsed_ms, uint64_t now_ms)
{
    const uint32_t t = (uint32_t)(elapsed_ms % 40000u);
    float rpm = 0.0f, voltage = 12.6f;
    model->demo = true;
    if (t >= 38000u) {
        tach_model_set_rpm(model, NAN, now_ms);
        tach_model_set_voltage(model, NAN, now_ms);
        return;
    }
    if (t >= 3000u && t < 9000u) {
        rpm = 850.0f + 15.0f * sinf((float)t / 500.0f);
        voltage = 13.4f;
    } else if (t >= 9000u && t < 18000u) {
        rpm = 850.0f + 3950.0f * (float)(t - 9000u) / 9000.0f;
        voltage = 13.8f;
    } else if (t >= 18000u && t < 26000u) {
        rpm = 4800.0f + 40.0f * sinf((float)t / 1100.0f);
        voltage = 14.1f;
    } else if (t >= 26000u && t < 32000u) {
        rpm = 4800.0f - 3950.0f * (float)(t - 26000u) / 6000.0f;
        voltage = 13.7f;
    } else if (t >= 32000u) {
        rpm = 850.0f;
        voltage = t < 35000u ? 11.6f : 15.6f;
    }
    tach_model_set_rpm(model, rpm, now_ms);
    tach_model_set_voltage(model, voltage, now_ms);
}
