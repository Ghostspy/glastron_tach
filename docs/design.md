# Boat tachometer display

The boat will have two digital gauges: RPM with charging voltage, and speed with fuel.
This iteration builds only the tachometer on the existing Waveshare
ESP32-S3-Touch-LCD-2.8C (480 x 480). The engine interface remains pending measurements.

Use ESP-IDF 5.4.4 and LVGL 8.3.11, with the vendor's ST7701S initialization sequence.
Keep the UI independent of the board and measurements so the exact UI can render on
the host. All LVGL calls belong to one task. The default build shows missing readings
until a sensor provider publishes data. The optional demo mode labels simulated readings.
No engine inputs, ADC channels, radios, SD card, or battery charger readings are enabled.

Visual direction: a marine instrument, with a black face (#000000), off-white
markings (#F5F6F2), blue progress (#65C7F0), muted gray (#819099), and amber
voltage warnings (#FFC15A). Use Barlow Condensed RPM numerals with Montserrat labels,
a 270-degree 0-7000 RPM scale,
large centered RPM, and voltage below. The scale endpoint is not an engine redline.
Keep all content inside the circular face. No unverified RPM limit is presented.

```
             3   4
         2           5
      1    MERCURY 115   6
              3250
      0        RPM      7
             13.8 V
             Battery
               DEMO
```

The alternatives were an Arduino sketch and adapting the adjacent automotive
cluster. ESP-IDF matches the nearby projects; using the vendor panel sequence and
a fresh UI avoids carrying over unrelated automotive protocols and graphics.

Startup: show the official Glastron website signature centered in white on black,
with `SSV-175` and a blue loading bar. Show it for 10 seconds measured from the
first visible frame, then switch to the gauge and release the startup objects.
Do not block the LVGL/watchdog loop, and do not let touch skip the delay. Start
the demo's elapsed time when the tachometer appears.

Reading timestamps use 64-bit monotonic milliseconds so lost samples stay expired
past the 32-bit clock boundary. Voltage bands in the demo are presentation examples, configurable and not a
diagnosis of this engine's charging system. Missing/invalid/stale values show
dashes and cannot silently hold the last good reading. Touch toggles day/night
brightness, with no touch dependency for the instrument to work.

Physical validation remains required: flash/boot, colors, touch alignment,
sunlight visibility, extended rendering, and eventually real engine inputs.
