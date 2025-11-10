#pragma once

#include <hbc/common.hpp>

/* Only include 'string.hpp' because 'util/fs.hpp' depends on this file. */
#include <hbc/util/string.hpp>

namespace hbc::config {

    namespace paths {

        constexpr inline util::fixed_string config_directory = HBC_CONFIG_DIR;

        constexpr inline util::fixed_string user_theme = paths::config_directory + "theme.nx.hbc";

        constexpr inline util::fixed_string base_theme = HBC_BASE_THEME_PATH;

        constexpr inline util::zstring apps_directory = HBC_APPS_DIR;

    }

    /* Just get the path separator from our path for the config directory. */
    constexpr inline char path_separator = paths::config_directory.back();

}
