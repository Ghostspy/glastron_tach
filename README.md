# Glastron / Mercury digital tachometer

Display firmware for the **Waveshare ESP32-S3-Touch-LCD-2.8C**, the round 480 x 480
model. This is the RPM + battery/charging-voltage gauge in a planned two-gauge
dashboard. Speed + fuel is a separate, later firmware project.

Boat: 1986 Glastron SSV-175. Engine: Mercury 115 HP, serial 6582437, identified
from the serial range as a 1984 inline six. Its expected six tach pulses per
revolution still require validation when the protected engine input is added.

## What works in this version

- A 10-second Glastron logo loading screen, then the tachometer. The countdown
  begins when the logo becomes visible; touches do not skip it. The event loop
  and watchdog continue running throughout startup.
- 0–7000 RPM scale, blue progress arc, large numeric RPM and battery voltage.
- Clearly marked **DEMO** mode, enabled by default. Its 40-second loop shows
  stopped, idle, acceleration, cruise, deceleration, low/high voltage, and missing data.
- Tap the tachometer face to toggle day/night brightness (100% / 20% by default).
- Independent missing/stale readings: dashes replace invalid values or samples
  older than two seconds. Zero RPM remains a valid stopped-engine reading.
- Actual LVGL host rendering, model tests, watchdog, and serial boot diagnostics.

**No boat signal is read yet.** The screen's voltage is simulated, not a reading
from the board's LiPo connector. There is no tach GPIO or boat-voltage ADC configured.
Test this version using USB power only. Keep the boat wiring disconnected until
the protected interface is designed and verified.

The 7000 RPM scale endpoint is a display range, **not the engine's maximum RPM or
redline**. Low (<12.0 V) / high (>15.0 V) bands are provisional UI examples, not
validated fault limits for this Mercury charging system. Change the two macros in
`main/tach_model.c` when actual measurements and charging specifications are known.

## Dependencies

Python helper-tool requirements are pinned in `requirements.txt`:

| Package | Version | Used for |
|---|---|---|
| PlatformIO | 6.2.0 | Command-line builds; also supplied by the VS Code extension |
| Pillow | 13.0.0 | Regenerating the numeric font and Glastron logo |
| esptool | 4.11.0 | Standalone firmware packaging and flashing |

Install these in a separate virtual environment, using Python 3.11 or newer:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

On Windows, activate with `.venv\Scripts\Activate.ps1` in PowerShell instead.
For the PlatformIO CLI, Python 3.11 matches the validated build environment.
Normal VS Code firmware builds do not require this helper environment: PlatformIO
manages its build tools, and the generated font/logo sources are already included.
The remaining Python imports are from the standard library, or SCons supplied by
PlatformIO; do not install SCons separately for the PlatformIO hook scripts.

ESP32 dependencies use the native ESP-IDF equivalent, `main/idf_component.yml`:

- **ESP-IDF 5.1.5**, also pinned in `platformio.ini`.
- **LVGL 8.3.11**, referenced by local path to `components/lvgl`; its version and
  license metadata are in `components/lvgl/idf_component.yml`.
- GPIO, I2C, SPI, LCD, timers and FreeRTOS come with ESP-IDF. Their component
  requirements are declared in `main/CMakeLists.txt`, with no separate downloads.

PlatformIO continues to compile the included LVGL source through CMake, so there
is intentionally no additional `lib_deps` download. The IDF manifest also supports
standalone IDF builds when the component manager is enabled.

## PlatformIO in VS Code

1. Install the **PlatformIO IDE** extension (`platformio.platformio-ide`) in VS Code.
2. Use **File → Open Folder** to open this `tach` directory, containing
   `platformio.ini`. Open the existing project rather than creating a new one.
3. Allow PlatformIO to download its pinned toolchain on first use. This project
   uses ESP-IDF 5.1.5; a separate ESP-IDF installation is not required. Tools and
   Python dependencies are kept in the project's ignored `.pio/core` directory
   so older shared SDK environments do not interfere.
4. Select the **tach** environment in the PlatformIO status bar, then click
   **Build** (checkmark).
5. Connect the board's **USB TO UART** Type-C port. Click **Upload** (arrow), then
   **Serial Monitor** (plug) to see logs at 115200 baud.

The supplied board definition selects the ESP32-S3, 16 MB flash and the existing
8 MB octal-PSRAM configuration. The ten-second Glastron loading screen and demo
tachometer are included. No graphics libraries or generated assets need installing.

You can also use **Terminal → Run Task** for the supplied PlatformIO Build,
Upload and Monitor tasks. PlatformIO supplies IntelliSense from the actual build;
do not enter machine-specific SDK include paths manually.

Equivalent commands in the **PlatformIO Core CLI** terminal:

```sh
pio run -e tach
pio run -e tach -t upload
pio device monitor -e tach
pio run -e tach -t menuconfig
```

Under **Boat tachometer** in menuconfig, change demo mode and brightness. PlatformIO
stores these settings in generated `sdkconfig.tach`, separate from the standalone
IDF build's `sdkconfig`. The tracked `sdkconfig.defaults` seeds a new configuration.
To return to defaults, remove `sdkconfig.tach` and clean/rebuild the `tach` environment.

Ports are autodetected. If multiple serial devices are connected, set `upload_port`
and `monitor_port` in `platformio.ini`, or supply `--upload-port` / `--port` to the
corresponding CLI commands. On Windows use the assigned `COM` port; on macOS use
the board's `/dev/cu.*` port. Close Serial Monitor before starting another program
that needs the same port. Exit this monitor with Ctrl+C.

Build products live in `.pio/build/tach/`. PlatformIO uploads the application,
bootloader and partition table at their correct offsets. Its `firmware.bin` is an
application image, unlike the combined offset-0 demo image in `dist/`.
Each relink automatically checks that LVGL's large memory arena is in PSRAM.
The PlatformIO build disables IDF registry resolution because its only graphics
dependency is vendored in `components/lvgl`. If you later add managed IDF
components, enable the component manager in `scripts/platformio_setup.py` first.

Configuration references: [PlatformIO ESP-IDF project layout](https://docs.platformio.org/en/latest/frameworks/espidf.html#project-structure)
and [PlatformIO VS Code tasks](https://docs.platformio.org/en/latest/integration/ide/vscode.html#task-runner).

## Standalone ESP-IDF build and prebuilt image

For a prebuilt USB demo, use `dist/boat-tach-demo-v0.1.1.zip` and its `FLASHING.txt`.
The combined image is written at offset **0x0**; it includes all three required
firmware segments. It has not been flashed or tested on the physical board.

Use **ESP-IDF 5.1.5**, an ESP32-S3 toolchain, and an activated IDF terminal.
LVGL 8.3.11 is included, so this project does not download a graphics library at
build time. No SD card, wireless connection, or LiPo battery is required.

```sh
idf.py set-target esp32s3
idf.py build
python3 scripts/check-memory.py
idf.py -p /dev/cu.YOUR_PORT flash monitor
```

Use the board's **USB TO UART** Type-C connector for this default UART0 console
and flashing setup. Select the port actually listed on your computer. Exit the
serial monitor with Ctrl+]. Flashing replaces the factory demo; download the vendor
demo first if you want to restore it later.

In `idf.py menuconfig` → **Boat tachometer**, change demo mode and brightness.
With demo disabled, the gauge intentionally displays `NO DATA`; there is no
automatic fallback to plausible-looking demo numbers.

## Host tests and display previews

Requires CMake, a C11 compiler and Ninja (or omit `-G Ninja`). The native tests
need no third-party Python packages. PlatformIO manages firmware-build Python
dependencies automatically.

```sh
sh scripts/test-model.sh
cmake -S tests -B build-host -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

`build-host/render-ui normal preview.ppm` renders the **same C/LVGL UI** as the
firmware. Scene names: `normal`, `stopped`, `low`, `high`, `missing`, `rpm-missing`,
`voltage-missing`, `night`, `startup`. The PPM output has gray outside the circular viewport;
the physical display shows only the round face. On macOS convert with:

```sh
sips -s format png preview.ppm --out preview.png
```

Checked PNGs are in `previews/`. Host rendering does not verify the physical LCD,
touch alignment, brightness, or sunlight readability.

## Code map and future sensor integration

| File | Purpose |
|---|---|
| `main/main.c` | Single-task event loop, demo provider and touch brightness |
| `main/board.c` | 2.8C pin map, RGB panel, GT911 touch and PWM backlight |
| `main/panel_init.h` | Official Waveshare ST7701S register initialization |
| `main/tach_model.c` | Input validity, independent sample ages and demo scenarios |
| `main/tach_ui.c` | Sensor-independent LVGL face |
| `main/tach_startup.c` | Nonblocking 10-second logo and screen transition |
| `main/glastron_logo.c` | Official website logo rasterized into a static LVGL asset |
| `main/tach_font_rpm.c` | Generated numeric font; no runtime font dependencies |

Once the engine interface is ready, disable demo and publish measured values on
the app task using `tach_model_set_rpm(&model, rpm, now_ms)` and
`tach_model_set_voltage(&model, volts, now_ms)` at the provider hook in `main.c`.
If acquisition runs in another task, transfer samples through a FreeRTOS queue;
do not call LVGL from that task. Stop publishing a channel on lost communication
so its displayed value expires. A healthy pulse-acquisition circuit must publish
zero RPM when it confirms the engine has stopped.

Voltage displayed from the ignition feed will be **helm supply voltage**; voltage
directly at the battery requires a dedicated protected sensing connection.

The port uses one PSRAM framebuffer, a 10-line RGB bounce buffer, and a 30-line
internal LVGL draw buffer. LVGL's 192 KiB arena is placed in external RAM. Partial
refresh keeps memory and bandwidth modest; physical tear/flicker testing remains.
Touch transactions are bounded; missing touch at boot leaves the gauge working.

## Hardware validation still required

This source can be compiled and rendered without a board. Before installation,
flash and verify boot, correct colors/orientation, continuous animation, touch
brightness and missing-data behavior. Check readability in the actual helm, then
complete the protected power/tach/voltage interface and compare against the meters
and original tach. Sensor accuracy has not been established by this display demo.

See `THIRD_PARTY.md` for upstream resources and licenses.
