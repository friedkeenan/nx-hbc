#!/usr/bin/env python3

import sys
import shutil
import zipfile
import libconf
import xml.etree.ElementTree as ET
from PIL import Image
from pathlib import Path

import gen_theme

asset_names = {
    "apps_list.png": ("apps_list.png", (648, 96)),
    "apps_list_hover.png": ("apps_list_hover.png", (648, 96)),
    "apps_next.png": ("apps_next.png", (96, 96)),
    "apps_next_hover.png": ("apps_next_hover.png", (96, 96)),
    "apps_previous.png": ("apps_previous.png", (96, 96)),
    "apps_previous_hover.png": ("apps_previous_hover.png", (96, 96)),
    "background_wide.png": ("background.png", (1280, 720)),
    "button_tiny.png": ("button_tiny.png", (175, 72)),
    "button_tiny_focus.png": ("button_tiny_focus.png", (175, 72)),
    "cursor_pic.png": ("cursor.png", (96, 96)),
    "dialog_background.png": ("dialog_background.png", (780, 540)),
    "logo.png": ("logo.png", (381, 34)),
    "icon_network_active.png": ("network_active.png", (48, 48)),
    "icon_network.png": ("network_inactive.png", (48, 48)),
    "progress.png": ("remote_progress.png", (600, 168)),
}

def color_from_elem(elem):
    r = int(elem.find("red").text)
    g = int(elem.find("green").text)
    b = int(elem.find("blue").text)

    return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF)

def main(argv):
    usage = "Usage: convert_theme.py <input theme dir> <output theme.zip>"

    if len(argv) < 2:
        print(usage)
        return -1

    in_dir = Path(argv[0])
    if not in_dir.is_dir():
        print("Invalid input directory")
        return -1

    tmp_dir = Path("tmp_convert")
    if not tmp_dir.exists():
        tmp_dir.mkdir()

    try:
        icon = Image.open(Path(in_dir, "icon.png"))
        icon = icon.resize((256, 256)).convert("RGB")
        icon.save(Path(tmp_dir, "icon.jpg"))
    except FileNotFoundError:
        pass

    cfg_info = {
        "info": {
        }
    }

    try:
        xml_info = ET.parse(Path(in_dir, "meta.xml")).getroot()

        name = xml_info.find("name")
        if name is None:
            cfg_info["info"]["name"] = in_dir.name
        else:
            cfg_info["info"]["name"] = name.text

        author = xml_info.find("coder")
        if author is None:
            cfg_info["info"]["author"] = "Unspecified Author"
        else:
            cfg_info["info"]["author"] = author.text

        version = xml_info.find("version")
        if version is None:
            cfg_info["info"]["version"] = "1.0"
        else:
            cfg_info["info"]["version"] = version.text

        with Path(tmp_dir, "info.cfg").open("w") as f:
            libconf.dump(cfg_info, f)
    except FileNotFoundError:
        pass

    try:
        with zipfile.ZipFile(Path(in_dir, "theme.zip"), "r") as zf:
            try:
                cfg_styles = {
                    "styles": {
                    }
                }

                with zf.open("theme.xml") as f:
                    xml_styles = ET.parse(f).getroot()

                    norm_text_col = xml_styles.find("font_color")
                    if norm_text_col is not None:
                        cfg_styles["styles"]["normal_text_color"] = color_from_elem(norm_text_col)

                    prog_grad = xml_styles.find("progress_gradient")
                    if prog_grad is not None:
                        main_col = [prog_grad.find("upper_left"), prog_grad.find("upper_right")]
                        main_col = [color_from_elem(x) for x in main_col if x is not None]
                        main_col = int(sum(main_col) / len(main_col))

                        grad_col = [prog_grad.find("lower_left"), prog_grad.find("lower_right")]
                        grad_col = [color_from_elem(x) for x in grad_col if x is not None]
                        grad_col = int(sum(grad_col) / len(grad_col))

                        cfg_styles["styles"]["remote_bar_main_color"] = main_col
                        cfg_styles["styles"]["remote_bar_grad_color"] = grad_col

                with Path(tmp_dir, "styles.cfg").open("w") as f:
                    libconf.dump(cfg_styles, f)
            except KeyError:
                pass

            for old_name, (new_name, new_size) in asset_names.items():
                try:
                    with zf.open(old_name, "r") as f:
                        im = Image.open(f)
                        im = im.resize(new_size)
                        im.save(Path(tmp_dir, new_name))
                except KeyError:
                    pass
    except FileNotFoundError:
        pass

    res = gen_theme.main([tmp_dir, argv[1]])
    if res != 0:
        return res

    shutil.rmtree(tmp_dir)

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))