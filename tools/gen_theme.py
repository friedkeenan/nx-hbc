#!/usr/bin/env python3

import sys
import os
import zipfile
from PIL import Image
from pathlib import Path

def add_asset_to_theme(zf, path, new_path):
    im = Image.open(path).convert("RGBA")

    # Convert to BGRA
    r, g, b, a = im.split()
    im = Image.merge("RGBA", (b, g, r, a))

    zf.writestr(new_path, im.tobytes())

def main(argv):
    usage = "Usage: gen_theme.py <resources folder> <output theme.zip>"

    if len(argv) < 2:
        print(usage)
        return 1

    res_dir = Path(argv[0])
    if not res_dir.is_dir():
        print(usage)
        return 1

    theme_path = Path(argv[1])
    if not theme_path.parent.exists():
        os.makedirs(theme_path.parent)

    with zipfile.ZipFile(theme_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for p in res_dir.iterdir():
            if p.suffix == ".cfg" or p.name == "icon.jpg":
                with p.open("rb") as f:
                    zf.writestr(p.name, f.read())
            else:
                add_asset_to_theme(zf, p, f"{p.stem}.bin")

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))