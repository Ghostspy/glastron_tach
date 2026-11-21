"""Verify LVGL's large arena is linked into PSRAM, leaving DMA memory available."""
import argparse
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("elf", type=Path, nargs="?", default=root / "build/boat_tach.elf")
parser.add_argument("--objdump", default="xtensa-esp32s3-elf-objdump")
args = parser.parse_args()
symbols = subprocess.check_output([args.objdump, "-t", str(args.elf)], text=True)
arenas = [line for line in symbols.splitlines() if line.split() and
          line.split()[-1].startswith("work_mem_int")]
if not arenas or not all(".ext_ram.bss" in line.split() for line in arenas):
    raise SystemExit("FAIL: LVGL arena is not in .ext_ram.bss: " + "; ".join(arenas))
print("PASS: LVGL arena is linked in external PSRAM")
