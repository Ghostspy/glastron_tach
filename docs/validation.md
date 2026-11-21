# Validation — 2026-11-20

## PlatformIO / VS Code support

- `pio run -e tach` passes using the tracked `platformio.ini`, custom Waveshare
  board manifest and project-local `.pio/core` tool cache, without shell environment
  overrides. Platform: Espressif 32 6.5.0; ESP-IDF 5.1.5; GCC 12.2.0.
- PlatformIO links the application, bootloader and partition table and produces
  `.pio/build/tach/firmware.bin`. The automatic post-link PSRAM check passes.
- `pio project metadata -e tach` generates compiler and include-path metadata for
  the IDE. VS Code task identifiers match the installed extension's native Build,
  Upload and Monitor task definitions; JSON configuration parses successfully.
- The existing native suite remains 11/11 passing.
- The first test cache was prepared from existing local compiler/SDK packages and
  temporary downloaded dependencies because network access is restricted in this
  session. Fresh online dependency download was not tested end to end.
- Upload and serial-monitor tasks are configured but no physical board was flashed
  or connected for this validation. No global SDK installation was changed.

## v0.1.1: Glastron startup

- `startup-timing` fails without the implementation and passes with it; verifies
  the visible-time origin, 0/5000/9999/10000 ms boundaries, safe repeated completion
  and long-uptime timestamps crossing the 32-bit boundary.
- Native suite: 11/11 pass, including the actual LVGL startup render.
- Demo and non-demo ESP-IDF builds pass with the startup feature. Memory-section
  checks continue to pass.
- `previews/glastron-startup.png` was visually inspected. The logo fits the round
  viewport and retains the official website signature's proportions.
- Read-only review found no important startup issues. No board has been flashed;
  physical visibility, ten-second timing and touch behavior remain to be checked.

## v0.1.0 baseline

Target: Waveshare ESP32-S3-Touch-LCD-2.8C. Toolchain: ESP-IDF 5.1.5,
Xtensa ESP32-S3 GCC 12.2.0. Graphics: LVGL 8.3.11.

Completed:

- `sh scripts/test-model.sh`: 7 groups pass with address and undefined-behavior
  sanitizers. Covers missing, stopped, invalid/nonfinite, independent stale
  readings, voltage bands, demo states and the 32-bit time boundary.
- `ctest --test-dir build-host --output-on-failure`: 9/9 checks pass. Eight scene
  renders use the actual firmware UI and also verify visible RPM pixels,
  static-buffer updates, stale transitions, cleared arcs and recovery.
- Demo and non-demo ESP-IDF builds pass. Neither config enables engine inputs.
- `scripts/check-memory.py`: LVGL's 192 KiB arena is in `.ext_ram.bss` in both
  firmware builds, leaving internal DMA-capable memory available.
- Normal, low-voltage, missing-data and night PNGs were visually inspected.
- Independent read-only review checked SPI/RGB startup against the official
  vendor demo. The identified sample-aging issue was fixed and regression-tested.
- Combined firmware offsets match the built bootloader, partition table and
  application. ZIP contents and image SHA-256 are verified.

The local build used existing SDK/compiler installations and temporary Python
dependencies outside the project. Normal source builds use an activated IDF
terminal as documented in README.md. No global SDK or Python installation was changed.

Not verified: physical boot, runtime heap/stack margins, panel colors/orientation,
touch alignment, sustained display performance, sunlight visibility, power
transients, moisture protection or sensor accuracy. No board was flashed and
no boat wiring was connected. These are hardware acceptance checks, not claims
established by compiling or rendering the display code.
