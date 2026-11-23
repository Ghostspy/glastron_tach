"""Use vendored IDF components for both app and bootloader configuration."""
import os
from pathlib import Path
import subprocess

Import("env")

# Retry an interrupted package install before CMake reads the framework.
framework = Path(env.PioPlatform().get_package_dir("framework-espidf"))
subprocess.run([env.subst("$PYTHONEXE"), str(framework / "install.py")], check=True)

# No managed components are used. Apply to every CMake subprocess, including the
# bootloader, which PlatformIO configures separately from the application.
os.environ["IDF_COMPONENT_MANAGER"] = "0"
env["ENV"]["IDF_COMPONENT_MANAGER"] = "0"
