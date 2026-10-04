#!/usr/bin/env python3
"""Link this checkout's board and library into a PlatformIO Core directory."""

import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--core-dir", type=Path,
        default=Path(os.environ.get("PLATFORMIO_CORE_DIR", "~/.platformio")).expanduser(),
        help="PlatformIO Core directory (default: PLATFORMIO_CORE_DIR or ~/.platformio)",
    )
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    core = args.core_dir.expanduser().resolve()
    pio = shutil.which("pio") or shutil.which("platformio")
    if not pio:
        candidate = Path.home() / ".platformio/penv/bin/pio"
        if candidate.is_file():
            pio = str(candidate)
        else:
            parser.error("PlatformIO CLI not found; install PlatformIO or add pio to PATH")
    links = {
        core / "boards/matrixbit.json": root / "boards/matrixbit.json",
        core / "lib/Matrixbit": root / "lib/Matrixbit",
    }
    # Check all destinations before changing anything. Never replace other installs.
    for destination, source in links.items():
        if (destination.exists() or destination.is_symlink()) and destination.resolve() != source:
            parser.error(f"{destination} already exists and points elsewhere")
    manifest = json.loads((root / "lib/Matrixbit/library.json").read_text())
    command = [pio, "pkg", "install", "--global", "--storage-dir", str(core / "lib")]
    for name, version in manifest["dependencies"].items():
        command.extend(["--library", f"{name}@{version}"])
    subprocess.run(command, check=True, env={**os.environ, "PLATFORMIO_CORE_DIR": str(core)})
    for destination, source in links.items():
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.is_symlink():
            destination.symlink_to(source, target_is_directory=source.is_dir())
        print(f"{destination} -> {source}")
    print("Installed. Use board = matrixbit and #include <matrixbit.h>.")
    print("Keep this checkout in place; these links track its updates.")


if __name__ == "__main__":
    main()
