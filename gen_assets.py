#!/usr/bin/env python3

import sys
from PIL import Image
from pathlib import Path

def asset_to_bin(path, new_path):
    im = Image.open(path).convert("RGBA")

    with new_path.open("wb") as f:
        f.write(im.tobytes())

if __name__ == "__main__":
    usage = "Usage: gen_assets.py <resources folder> <assets folder>"

    if len(sys.argv) < 3:
        print(usage)
        sys.exit(1)

    res_dir = Path(sys.argv[1])
    if not res_dir.is_dir():
        print(usage)
        sys.exit(1)

    assets_dir = Path(sys.argv[2])
    if not assets_dir.exists():
        assets_dir.mkdir()

    for p in res_dir.glob("*"):
        asset_to_bin(p, Path(assets_dir, f"{p.stem}.bin"))