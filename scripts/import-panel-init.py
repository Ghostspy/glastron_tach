"""Extract the 2.8C register sequence from the official Waveshare demo ZIP."""
from pathlib import Path
import re
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
with zipfile.ZipFile(sys.argv[1]) as archive:
    source = archive.read(
        "ESP-IDF/ESP32-S3-Touch-LCD-2.8C-Test/main/LCD_Driver/ST7701S.c"
    ).decode()
source = source.split("void ST7701S_screen_init(", 1)[1].split("void ST7701S_delObject(", 1)[0]
commands = []
for match in re.finditer(r"SPI_Write(Comm|Data)\(0x([0-9a-fA-F]+)\)|Delay\((\d+)\)", source):
    kind, value, delay = match.groups()
    if delay:
        commands[-1][2] = int(delay)
    elif kind == "Comm":
        commands.append([int(value, 16), [], 0])
    else:
        commands[-1][1].append(int(value, 16))
assert commands and commands[-1][0] == 0x29
assert max(len(data) for _, data, _ in commands) <= 16
rows = []
for command, data, delay in commands:
    values = ", ".join(f"0x{value:02x}" for value in data) or "0"
    rows.append(f"    {{0x{command:02x}, {len(data)}, {{{values}}}, {delay}}}")
header = '''/* Waveshare ESP32-S3-Touch-LCD-2.8C ST7701S panel register sequence.
 * Extracted from official demo, downloaded 2026-11-20. See THIRD_PARTY.md.
 * Regenerate: python3 scripts/import-panel-init.py /path/to/demo.zip
 */
static const panel_command_t panel_commands[] = {
''' + ",\n".join(rows) + "\n};\n"
(root / "main/panel_init.h").write_text(header)
print(f"Imported {len(commands)} panel commands")
