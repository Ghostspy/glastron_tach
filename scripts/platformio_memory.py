"""Run the existing PSRAM-placement check after PlatformIO links the firmware."""
from pathlib import Path
import os
import subprocess

Import("env")


def check_memory(source, target, env):
    toolchain = Path(env.PioPlatform().get_package_dir("toolchain-xtensa-esp32s3"))
    objdump = toolchain / "bin" / ("xtensa-esp32s3-elf-objdump" + (".exe" if os.name == "nt" else ""))
    subprocess.run(
        [env.subst("$PYTHONEXE"),
         str(Path(env.subst("$PROJECT_DIR")) / "scripts/check-memory.py"),
         str(target[0]), "--objdump", str(objdump)],
        check=True,
    )


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", check_memory)
