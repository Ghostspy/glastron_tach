# Tachometer Display Implementation Plan

> **For agentic workers:** Execute inline in this empty workspace. There is no Git repository to branch or commit.

**Goal:** Build a USB-powered, demonstrable tachometer UI without connecting the boat.

**Architecture:** A portable measurement model supplies a portable LVGL UI. A thin
ESP-IDF board layer handles panel, touch, timing, and backlight. A host renderer uses
the same UI to produce reviewable images.

**Tech Stack:** C11, ESP-IDF 5.1.5, LVGL 8.3.11, CMake, native Clang tests.

**Spec:** `docs/design.md`

## Global Constraints

- Target only ESP32-S3-Touch-LCD-2.8C, 480 x 480.
- Simulated values are visibly marked DEMO; real inputs remain disabled.
- Single task owns all LVGL operations; no sensor GPIO assignments yet.
- Scale endpoint is not an engine redline; voltage warnings are provisional.

## Review Focus

- Missing RPM must not erase valid voltage, and vice versa.
- Stale, invalid, negative and nonfinite readings must become unavailable.
- Demo time rollover must not freeze animation or freshness checks.
- Circular edges must not clip labels or voltage status messages.
- A missing touch controller must not prevent gauge display.

### Task 1: Portable reading model

Files: `main/tach_model.h`, `main/tach_model.c`, `tests/test_model.c`.
Interface: independently timestamped RPM and voltage samples; `tach_model_view`
returns safe display values and status; `tach_demo_sample` produces marked demo data.

- [x] Write tests for missing, zero, valid, invalid, stale, independent timestamps,
  voltage bands and wraparound; run them and observe failure.
- [x] Implement the model and rerun with strict compiler warnings and sanitizers.

### Task 2: LVGL face and real host rendering

Files: `main/tach_ui.h`, `main/tach_ui.c`, `tests/render_ui.c`, `tests/CMakeLists.txt`.
Consumes: the model view. Produces: one screen, one update function, theme toggle.

- [x] Render normal, low/high voltage, missing and night states with the actual UI.
- [x] Inspect images and verify labels fit the circular viewport.

### Task 3: Board integration and firmware build

Files: `main/board.*`, `main/panel_init.h`, `main/main.c`, root build/config files,
`README.md`, `THIRD_PARTY.md`, helper scripts.

- [x] Integrate official panel initialization, checked I2C/SPI calls, touch fallback,
  PWM brightness, one LVGL loop, watchdog, reset logging and demo configuration.
- [x] Compile the firmware; rerun host checks and render snapshots.
- [x] Document build/flash and the outstanding physical validation.

## Completion evidence

- Seven model test groups pass with AddressSanitizer/UndefinedBehaviorSanitizer.
- All nine CTest checks pass, including eight real LVGL render scenes.
- ESP-IDF 5.1.5 builds pass with demo enabled and disabled.
- A read-only reviewer checked the board setup against the vendor ZIP and found
  a 32-bit freshness revival edge. A failing regression test reproduced it; the
  model and app now use 64-bit monotonic timestamps and the full suite passes.
- Pixel inspection caught the scaled-label rendering issue; a visible-pixel
  regression test now guards the dedicated numeric font.
- The ELF memory-section check failed with the original arena attribute and
  passes with the correct `LV_ATTRIBUTE_LARGE_RAM_ARRAY` PSRAM attribute, in both modes.
- A merged image and ZIP were produced. Physical hardware remains untested.
