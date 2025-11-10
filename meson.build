project('nx-hbc', 'cpp',
    default_options : [
        # -O3
        'buildtype=release',

        'b_lto=true',

        # Notably, this will prevent STL assertions as well.
        'b_ndebug=if-release',

        'cpp_rtti=false',
        'cpp_eh=none',
    ],
)

name    = 'Homebrew Channel'
author  = 'friedkeenan, fail0verflow'
version = '0.1.0'

if host_machine.system() == 'horizon'
    dkp = run_command('sh', '-c', 'echo -n $DEVKITPRO', check: true).stdout()
    if dkp == ''
        error('Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro')
    endif
endif

# NOTE: We don't include the 'info.toml' or 'icon.jpg' files
# in the base theme since they are not required for base functionality.
#
# They exist in the 'theme' directory to serve as examples.
theme_files = files(
    'theme/app_button.png',
    'theme/app_button_focus.png',
    'theme/background.png',
    'theme/dialog_background.png',
    'theme/font.ttf',
    'theme/logo.png',
    'theme/next_page_button.png',
    'theme/next_page_button_focus.png',
    'theme/previous_page_button.png',
    'theme/previous_page_button_focus.png',
    'theme/star_small.png',
    'theme/star_big.png',
    'theme/tiny_button.png',
    'theme/tiny_button_focus.png',
    'theme/unavailable_icon_big.png',
    'theme/unavailable_icon_small.png',
)

python = find_program('python3')

theme = custom_target(
    'theme',

    output: 'theme.nx.hbc',
    input:  theme_files,

    command: [python, files('tools/build_theme.py'), '-o', '@OUTPUT@', '@INPUT@'],

    build_by_default: true,
)

fs = import('fs')

conf = configuration_data()

if host_machine.system() == 'horizon'
    conf.set_quoted('BASE_THEME_PATH', 'romfs:/' + fs.name(theme.full_path()))

    conf.set_quoted('CONFIG_DIR', 'sdmc:/config/nx-hbc/')

    conf.set_quoted('APPS_DIR', 'sdmc:/switch/')
else
    conf.set_quoted('BASE_THEME_PATH', theme.full_path())

    conf.set_quoted('CONFIG_DIR', meson.current_build_dir() / 'config/')

    conf.set_quoted('APPS_DIR', meson.current_build_dir() / 'apps/')
endif

conf.set('LV_CONF_PATH', files('include/lv_conf.h')[0].full_path())

sanitizers = [
    # '-fsanitize=address',
    # '-fsanitize=undefined',
]

if host_machine.system() == 'horizon'

    link_args = [
        '-specs=' + dkp + '/libnx/switch.specs',
    ]

else

    link_args = [
        '-s',
    ]

endif

cpp_args = [
    '-std=gnu++2c',

    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wconversion',
    '-Werror',

    '-fconcepts-diagnostics-depth=3',

    '-DHBC_BASE_THEME_PATH=' + conf.get('BASE_THEME_PATH'),

    '-DHBC_CONFIG_DIR=' + conf.get('CONFIG_DIR'),

    '-DHBC_APPS_DIR=' + conf.get('APPS_DIR'),
]

link_args += sanitizers
cpp_args  += sanitizers

cmake = import('cmake')

fmt_proj = cmake.subproject('fmt')

glaze_options = cmake.subproject_options()

glaze_options.add_cmake_defines({
    'glaze_DEVELOPER_MODE': 0,
    'glaze_ENABLE_FUZZING': 0,
})

glaze_proj = cmake.subproject('glaze', options: glaze_options)

lvgl_options = cmake.subproject_options()

lvgl_options.add_cmake_defines({
    'LV_BUILD_CONF_PATH': conf.get('LV_CONF_PATH'),

    'CONFIG_LV_USE_THORVG_INTERNAL': 0,

    'CONFIG_LV_BUILD_EXAMPLES': 0,

    'CONFIG_LV_BUILD_DEMOS': 0,
})

lvgl_proj = cmake.subproject('lvgl', options: lvgl_options)

dependencies = [
    dependency('libturbojpeg'),
    dependency('minizip'),

    fmt_proj.dependency('fmt'),
    glaze_proj.dependency('glaze_glaze'),
    lvgl_proj.dependency('lvgl'),
]

if host_machine.system() != 'horizon'

    dependencies += dependency('sdl2')

endif

sources = []
subdir('source')

exe = executable(meson.project_name(), sources,
    include_directories: 'include',

    dependencies: dependencies,

    link_args: link_args,
    cpp_args:  cpp_args,
)

if host_machine.system() == 'horizon'
    nacptool = find_program(dkp + '/tools/bin/nacptool')

    nacp = custom_target(
        'nacp',

        output: exe.name() + '.nacp',
        input:  [],

        command: [nacptool, '--create', name, author, version, '@OUTPUT@'],
    )

    romfs = custom_target(
        'romfs',

        output: 'romfs',
        input:  theme,

        command: ['mkdir', '-p', '@OUTPUT@', '&&', 'cp', '@INPUT@', '@OUTPUT@']
    )

    elf2nro = find_program(dkp + '/tools/bin/elf2nro')

    icon = files('icon.jpg')[0].full_path()

    custom_target(
        'nro',

        output: exe.name() + '.nro',
        input: [exe, icon, nacp, romfs],

        command: [
            elf2nro,

            exe,
            '@OUTPUT@',

            '--icon='     + icon,
            '--nacp='     + nacp.full_path(),
            '--romfsdir=' + romfs.full_path(),
        ],

        build_by_default: true
    )
endif
