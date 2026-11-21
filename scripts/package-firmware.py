"""Package a compiled demo as one flashable image; run from an IDF environment."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build", type=Path, default=root / "build")
parser.add_argument("--esptool", type=Path, help="Optional esptool.py entry point")
parser.add_argument("--objdump", default="xtensa-esp32s3-elf-objdump")
args = parser.parse_args()
build = args.build.resolve()
config = json.loads((build / "config/sdkconfig.json").read_text())
if not config.get("TACH_DEMO_MODE"):
    raise SystemExit("Refusing to label a non-demo build as demo firmware")
if config.get("IDF_TARGET") != "esp32s3":
    raise SystemExit("Expected ESP32-S3 firmware")
flash = json.loads((build / "flasher_args.json").read_text())
subprocess.run([sys.executable, str(root / "scripts/check-memory.py"),
                str(build / "boat_tach.elf"), "--objdump", args.objdump], check=True)
version = json.loads((build / "project_description.json").read_text())["project_version"]
name = f"boat-tach-demo-v{version}"
dist = root / "dist"
dist.mkdir(exist_ok=True)
image = dist / f"{name}.bin"
prefix = [sys.executable, str(args.esptool)] if args.esptool else [sys.executable, "-m", "esptool"]
command = prefix + ["--chip", "esp32s3", "merge_bin", "-o", str(image)] + flash["write_flash_args"]
for offset, filename in sorted(flash["flash_files"].items(), key=lambda pair: int(pair[0], 0)):
    command += [offset, filename]
subprocess.run(command, cwd=build, check=True)
digest = hashlib.sha256(image.read_bytes()).hexdigest()
checksums = dist / f"{name}.sha256"
checksums.write_text(f"{digest}  {image.name}\n")
instructions = f'''Boat tachometer USB demo v{version}

Target: Waveshare ESP32-S3-Touch-LCD-2.8C (round 480 x 480), ESP32-S3, 16 MB flash.
DEMO readings only. Boat/engine wiring must remain disconnected. USB power only.
The Glastron loading screen stays visible for 10 seconds before the tach appears.
Tap the tach face to toggle brightness. The 40-second cycle includes missing readings.

This combined image includes bootloader, partition table and application.
Write it at offset 0x0 (not 0x10000). Flashing replaces the board's existing firmware.
Use the USB TO UART connector. No flash operation has been performed by the agent.

From a terminal with esptool installed (e.g. an activated ESP-IDF terminal):

python -m esptool --chip esp32s3 --port YOUR_PORT write_flash 0x0 {image.name}

Select YOUR_PORT from your computer's serial-device list. If automatic download
fails, hold BOOT, press and release RESET, then release BOOT and retry.
After flashing, press RESET if the demo does not start automatically.

The firmware compiles and its actual LVGL layout has been rendered on a computer.
Physical screen/touch/brightness and engine measurements remain unverified.
See the source project's README.md for source builds and hardware validation.

SHA-256: {digest}
'''
readme = dist / "FLASHING.txt"
readme.write_text(instructions)
with zipfile.ZipFile(dist / f"{name}.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    for file in [image, checksums, readme]:
        archive.write(file, file.name)
print(f"Packaged {image.name}: {image.stat().st_size} bytes; sha256 {digest}")
