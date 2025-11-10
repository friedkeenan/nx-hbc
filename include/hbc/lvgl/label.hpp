#pragma once

#include <hbc/common.hpp>

#include <hbc/lang/lang.hpp>

#include <hbc/lvgl/object.hpp>

namespace hbc::lvgl {

    struct label : lvgl::object {
        label(lvgl::parentable auto &parent) : lvgl::object(parent, lv_label_create) {}

        void set_text_translated(this label &self, const lang::translated_view text) {
            lv_label_set_text(self._object, text.c_str());
        }

        inline void set_long_mode(this label &self, const lv_label_long_mode_t mode) {
            lv_label_set_long_mode(self._object, mode);
        }
    };

}
