#pragma once

#include <cstddef>
#include <cstdint>

#include <stdfloat>
#include <compare>
#include <bit>
#include <limits>
#include <atomic>
#include <utility>
#include <functional>
#include <memory>
#include <string_view>
#include <array>
#include <variant>
#include <optional>
#include <expected>
#include <algorithm>
#include <concepts>
#include <iterator>
#include <ranges>

#include <dirent.h>
#include <sys/stat.h>

#include <lvgl.h>

#if defined(__SWITCH__)

#include <switch.h>

#else

#include <SDL2/SDL.h>

#endif

#include <fmt/format.h>

/*
    GCC was warning about conversions within Glaze.

    Presumably, we do not need to worry about those.
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

#include <glaze/glaze.hpp>
#include <glaze/toml.hpp>

#pragma GCC diagnostic pop

#include <turbojpeg.h>
