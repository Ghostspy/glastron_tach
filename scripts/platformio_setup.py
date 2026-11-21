"""Use vendored IDF components for both app and bootloader configuration."""
import os

Import("env")

# No managed components are used. Apply to every CMake subprocess, including the
# bootloader, which PlatformIO configures separately from the application.
os.environ["IDF_COMPONENT_MANAGER"] = "0"
env["ENV"]["IDF_COMPONENT_MANAGER"] = "0"
