# Glastron startup screen

Requested change: show the Glastron logo for 10 seconds, then enter the existing tach.
Use the official website's signature glyph, centered in white on black, with a
small blue loading bar. No controls skip the startup period. Start the timer after
the first frame is drawn and the backlight is enabled. Continue LVGL and watchdog
service; use 64-bit monotonic time. Begin the demo cycle when the gauge appears.

- [x] Add timing/transition tests at 0, 9999, and 10000 ms, including long uptime.
- [x] Add the logo asset, portable LVGL startup screen, and main-loop transition.
- [x] Render and inspect the startup screen; build both modes and package v0.1.1.

Evidence: startup test failed before implementation, then passed. Full native
suite is 11/11. Demo and non-demo ESP-IDF builds pass; memory placement checks
pass. The actual LVGL startup render is `previews/glastron-startup.png`.
Read-only review found no important issues. Physical timing remains unverified.
