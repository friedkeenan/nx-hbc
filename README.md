# nx-hbc

A homebrew menu for the Nintendo Switch which mimics the Wii Homebrew Channel.

> [!CAUTION]
>
> **This project is currently in an incomplete state.**
>
> It is *functional*, but is missing some very important features, such as receiving input from controllers. I would not currently advise anyone to use this as a replacement for their current homebrew menu.
>
> In the meantime, to try it out, you can run nx-hbc just like any other homebrew app, by putting it in the `/switch/` directory on your SD card, and launching it from your normal homebrew menu.

This is not a port of the Wii Homebrew Channel, but rather more a rewrite. The code has been written from the ground up, and uses the graphics library LVGL in order to achieve the same look as the Wii Homebrew Channel.

## TODOs

- Controller input.
- Animation.
- Music.
- Gyro pointer.
- Better language support.
  - At least the following languages are needed for a 1.0.0 release:
    - Spanish.
    - German.
    - Portuguese.
- File associations.
- Configurable settings.
- Make a final determination on how best to delete "subdirectory" apps.
- Netloader.
- A PC tool to convert Wii Homebrew Channel themes to themes for nx-hbc.
- Miscellaneous UI stuff.
- Probably other things.

## Custom Themes

Custom themes are supported. Themes are packaged into a single file with the extension `.nx.hbc`, and are placed in the same directory as other homebrew apps.

Themes can be built with the [build_theme.py](https://github.com/friedkeenan/nx-hbc/blob/rewrite/tools/build_theme.py) tool. For example, you could create a copy of the bundled base theme with the following command (executed from the root of this repository):
```sh
./tools/build_theme.py -o my_theme.nx.hbc theme
```

After running that command, you should have a `my_theme.nx.hbc` file built using the contents of the [theme](https://github.com/friedkeenan/nx-hbc/tree/rewrite/theme) directory.

That directory also serves as an example theme. When making your own theme, make sure to use the same exact file names, including the file extensions.

## Building

This project can be built for the Nintendo Switch, and for PC for testing purposes.

The Meson build system is used for the build system of this project, and is required. By extension, the Ninja backend is also required.


### For the Nintendo Switch

Building for the Switch requires the following dependencies:
```
dkp-meson-scripts
dkp-toolchain-vars
switch-libjpeg-turbo
switch-zlib
```

After installing the above dependencies, you may run the following command:

```sh
$DEVKITPRO/meson-cross.sh switch switch.txt build
```

This will generate the proper "crossfile" for Meson for the Switch platform at `switch.txt`, and then use that to setup the build directory at `build`.

From there, you can `cd` into the `build` directory, and simply run `ninja`. By the end, you should be left with an `nx-hbc.nro` file in the same directory.

### For PC

Building for PC requires libjpeg-turbo, minizip, and SDL2. These are provided by the following Arch Linux packages:
```
libjpeg-turbo
minizip
sdl2
```

After installing the above dependencies, you can simply run the following command:
```sh
meson setup build
```

This will setup the build directory at `build`. From there, you can `cd` into the `build` directory and simply run `ninja`. By the end, you should be left with a plain `nx-hbc` executable.

Additionally, the PC build has specific defined paths which correspond to the directories the Switch version would use. If the build directory is `buiild`, then:
- The base theme path is at `build/theme.nx.hbc`, corresponding to `romfs:/theme.nx.hbc` on the Switch.
- The config directory is at `build/config`, corresponding to `sdmc:/config/nx-hbc` on the Switch.
- The apps directory is at `build/apps`, corresponding to `sdmc:/switch` on the Switch.

## Credits

- [fail0verflow/hbc](https://github.com/fail0verflow/hbc) for the inspiration, the assets, and the UI design.
- [switchbrew/nx-hbmenu](https://github.com/switchbrew/nx-hbmenu) for establishing the standards of homebrew applications on the Switch.
- [lvgl/lvgl](https://github.com/lvgl/lvgl) which we use for our graphics framework.
- [stephenberry/glaze](https://github.com/stephenberry/glaze) which we use for TOML parsing.
- [fmtlib/fmt](https://github.com/fmtlib/fmt) which we use for string formatting.
