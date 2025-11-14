#pragma once

#include <hbc/common.hpp>

namespace hbc::lvgl {

    struct color {
        lv_color_t _color;

        static inline color hex(const std::uint32_t value) {
            return {lv_color_hex(value)};
        }

        static inline color white() {
            return {lv_color_white()};
        }

        static inline color black() {
            return {lv_color_black()};
        }

        inline lv_color_t underlying(this color self) {
            return self._color;
        }
    };

}
