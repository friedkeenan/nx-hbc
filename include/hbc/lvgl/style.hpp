#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/color.hpp>

namespace hbc::lvgl {

    struct style {
        HBC_NON_COPYABLE(style);
        HBC_NON_MOVEABLE(style);

        lv_style_t _style;

        inline style() {
            lv_style_init(&this->_style);
        }

        inline ~style() {
            lv_style_reset(&this->_style);
        }

        inline const lv_style_t &underlying(this const style &self) {
            return self._style;
        }

        inline void set_background_color(this style &self, lvgl::color color) {
            lv_style_set_bg_color(&self._style, color.underlying());
        }

        inline void set_text_color(this style &self, lvgl::color color) {
            lv_style_set_text_color(&self._style, color.underlying());
        }

        inline void set_text_font(this style &self, const lv_font_t &font) {
            lv_style_set_text_font(&self._style, &font);
        }
    };

}
