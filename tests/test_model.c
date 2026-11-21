#include "tach_model.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

static void missing_and_zero(void)
{
    tach_model_t m = {0};
    tach_view_t v = tach_model_view(&m, 0);
    assert(!v.rpm_valid && !v.voltage_valid && !v.demo);
    tach_model_set_rpm(&m, 0, 0);
    v = tach_model_view(&m, 0);
    assert(v.rpm_valid && v.rpm == 0 && v.arc_rpm == 0);
}

static void invalid_values(void)
{
    tach_model_t m = {0};
    const float bad_rpm[] = {-1, NAN, INFINITY, 15001};
    const float bad_v[] = {-1, NAN, INFINITY, 0, 40};
    for (unsigned i = 0; i < sizeof bad_rpm / sizeof *bad_rpm; ++i) {
        tach_model_set_rpm(&m, 3000, 100);
        tach_model_set_rpm(&m, bad_rpm[i], 101);
        assert(!tach_model_view(&m, 101).rpm_valid);
    }
    for (unsigned i = 0; i < sizeof bad_v / sizeof *bad_v; ++i) {
        tach_model_set_voltage(&m, 13.8f, 100);
        tach_model_set_voltage(&m, bad_v[i], 101);
        assert(!tach_model_view(&m, 101).voltage_valid);
        assert(tach_model_view(&m, 101).voltage_status == TACH_VOLTAGE_UNKNOWN);
    }
}

static void stale_independently(void)
{
    tach_model_t m = {0};
    tach_model_set_rpm(&m, 3254, 10);
    tach_model_set_voltage(&m, 13.8f, 1010);
    tach_view_t v = tach_model_view(&m, 2010);
    assert(v.rpm_valid && v.voltage_valid);
    v = tach_model_view(&m, 2011);
    assert(!v.rpm_valid && v.voltage_valid);
    v = tach_model_view(&m, 3011);
    assert(!v.rpm_valid && !v.voltage_valid);
    tach_model_set_rpm(&m, 8400, 3012);
    v = tach_model_view(&m, 3012);
    assert(v.rpm_valid && v.rpm == 8400 && v.arc_rpm == TACH_SCALE_MAX_RPM);
}

static void wraparound(void)
{
    tach_model_t m = {0};
    tach_model_set_rpm(&m, 900, UINT32_MAX - 999);
    assert(tach_model_view(&m, (uint64_t)UINT32_MAX + 1001).rpm_valid);
    assert(!tach_model_view(&m, (uint64_t)UINT32_MAX + 1002).rpm_valid);
}

static void voltage_bands(void)
{
    tach_model_t m = {0};
    tach_model_set_voltage(&m, 11.9f, 0);
    assert(tach_model_view(&m, 0).voltage_status == TACH_VOLTAGE_LOW);
    tach_model_set_voltage(&m, 12.0f, 0);
    assert(tach_model_view(&m, 0).voltage_status == TACH_VOLTAGE_NORMAL);
    tach_model_set_voltage(&m, 15.0f, 0);
    assert(tach_model_view(&m, 0).voltage_status == TACH_VOLTAGE_NORMAL);
    tach_model_set_voltage(&m, 15.1f, 0);
    assert(tach_model_view(&m, 0).voltage_status == TACH_VOLTAGE_HIGH);
}

static void demo_is_explicit(void)
{
    tach_model_t m = {0};
    bool saw_low = false, saw_high = false, saw_missing = false;
    for (uint32_t t = 0; t < 48000; t += 40) {
        tach_demo_sample(&m, t, t);
        tach_view_t v = tach_model_view(&m, t);
        assert(v.demo);
        if (v.rpm_valid) assert(v.rpm >= 0 && v.rpm <= TACH_SCALE_MAX_RPM);
        saw_low |= v.voltage_status == TACH_VOLTAGE_LOW;
        saw_high |= v.voltage_status == TACH_VOLTAGE_HIGH;
        saw_missing |= !v.rpm_valid && !v.voltage_valid;
    }
    assert(saw_low && saw_high && saw_missing);
    tach_demo_sample(&m, 100, UINT32_MAX - 10);
    assert(tach_model_view(&m, (uint64_t)UINT32_MAX + 20).rpm_valid);
}

static void stale_data_never_revives(void)
{
    tach_model_t m = {0};
    tach_model_set_rpm(&m, 3000, 10);
    tach_model_set_voltage(&m, 13.8f, 10);
    uint64_t later = UINT64_C(0x100000000) + 10;
    assert(!tach_model_view(&m, later).rpm_valid);
    assert(!tach_model_view(&m, later).voltage_valid);
}

int main(void)
{
    missing_and_zero();
    invalid_values();
    stale_independently();
    wraparound();
    voltage_bands();
    demo_is_explicit();
    stale_data_never_revives();
    puts("PASS: 7 model test groups");
    return 0;
}
