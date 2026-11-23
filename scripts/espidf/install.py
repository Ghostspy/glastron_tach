"""Install Espressif's complete release; GitHub source zips omit submodules.

The upstream archive has no PlatformIO manifest. This small package supplies it,
verifies the official release checksum, and installs only the SDK build inputs.
"""
import hashlib
from pathlib import Path, PurePosixPath
import shutil
import ssl
import urllib.request
import zipfile

import certifi

VERSION = "5.4.4"
SHA256 = "906eeceaa7fe3a5eb7478ef6bf16f5640dabd64766d55bfe57d5ff36ce04f1dd"
URL = f"https://github.com/espressif/esp-idf/releases/download/v{VERSION}/esp-idf-v{VERSION}.zip"
INSTALL_STAMP = SHA256 + "\nbuild-inputs-v2\n"


def checksum(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def install():
    destination = Path(__file__).resolve().parent
    marker = destination / ".release-sha256"
    if marker.is_file() and marker.read_text() == INSTALL_STAMP:
        return

    # PlatformIO installs this package under <core_dir>/packages/framework-espidf.
    cache = destination.parents[1] / ".cache" / "downloads"
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / f"esp-idf-v{VERSION}.zip"
    if not archive.is_file() or checksum(archive) != SHA256:
        print(f"Downloading official ESP-IDF {VERSION} (about 2 GB, cached)...", flush=True)
        partial = archive.with_suffix(".zip.part")
        # Use PlatformIO's CA bundle; its portable Python can lack system CAs.
        context = ssl.create_default_context(cafile=certifi.where())
        with urllib.request.urlopen(URL, timeout=120, context=context) as response, partial.open("wb") as output:
            shutil.copyfileobj(response, output, length=1024 * 1024)
        if checksum(partial) != SHA256:
            raise RuntimeError("ESP-IDF release checksum mismatch; rebuild to retry")
        partial.replace(archive)

    print(f"Installing ESP-IDF {VERSION} components and tools...", flush=True)
    with zipfile.ZipFile(archive) as release:
        for member in release.infolist():
            parts = PurePosixPath(member.filename).parts
            if not parts or parts[0] != f"esp-idf-v{VERSION}":
                continue
            relative = PurePosixPath(*parts[1:])
            if not relative.parts or ".." in relative.parts:
                continue
            # Exclude the bundled .git objects, examples and documentation.
            if relative.parts[0] not in ("components", "tools") and (len(relative.parts) > 1 or member.is_dir()):
                continue
            target = destination.joinpath(*relative.parts)
            if member.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with release.open(member) as source, target.open("wb") as output:
                    shutil.copyfileobj(source, output)
                mode = (member.external_attr >> 16) & 0o777
                if mode:
                    target.chmod(mode)
    (destination / "version.txt").write_text(VERSION + "\n")
    marker.write_text(INSTALL_STAMP)


if __name__ == "__main__":
    install()
