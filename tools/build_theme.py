#!/usr/bin/env python3

import zipfile

from pathlib import Path
from PIL     import Image

def add_to_theme(zf, input):
    if input.is_dir():
        raise ValueError(f"The directory '{input}' cannot be added to a theme")

    if input.suffix != ".png":
        zf.write(input, arcname=input.name)

        return

    with input.open("rb") as f:
        im = Image.open(f).convert("RGBA")

        r, g, b, a = im.split()

        # LVGL consumes the colors in this order.
        im = Image.merge("RGBA", (b, g, r, a))

        zf.writestr(input.with_suffix(".bin").name, im.tobytes())

def generate_theme(output, inputs):
    output.parent.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as zf:
        if len(inputs) == 1 and inputs[0].is_dir():
            for input in inputs[0].iterdir():
                add_to_theme(zf, input)

            return

        for input in inputs:
            add_to_theme(zf, input)

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description = "Generate the usable theme files for nx-hbc"
    )

    parser.add_argument(
        "-o", "--output",

        type     = Path,
        required = True,
        help     = "The output directory",
    )

    parser.add_argument(
        "inputs",

        type  = Path,
        nargs = "*",
        help  = "The input files and directories for the theme",
    )

    args = parser.parse_args()

    generate_theme(args.output, args.inputs)
