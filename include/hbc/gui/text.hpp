#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/lvgl.hpp>

#include <hbc/gui/theme.hpp>

namespace hbc::gui {

    struct themed_text : lvgl::label {
        themed_text(lvgl::parentable auto &parent, const lvgl::style &style)
        :
            lvgl::label(parent)
        {
            this->add_style(style);

            this->set_long_mode(LV_LABEL_LONG_MODE_SCROLL);
        }
    };

    struct normal_16_text : gui::themed_text {
        static constexpr std::int32_t text_height = 16;

        normal_16_text(lvgl::parentable auto &parent, const gui::theme &theme)
        :
            gui::themed_text(parent, theme.normal_16_text.style())
        {}
    };

    struct normal_28_text : gui::themed_text {
        static constexpr std::int32_t text_height = 28;

        normal_28_text(lvgl::parentable auto &parent, const gui::theme &theme)
        :
            gui::themed_text(parent, theme.normal_28_text.style())
        {}
    };

    struct normal_48_text : gui::themed_text {
        static constexpr std::int32_t text_height = 48;

        normal_48_text(lvgl::parentable auto &parent, const gui::theme &theme)
        :
            gui::themed_text(parent, theme.normal_48_text.style())
        {}
    };

    struct warn_48_text : gui::themed_text {
        static constexpr std::int32_t text_height = 48;

        warn_48_text(lvgl::parentable auto &parent, const gui::theme &theme)
        :
            gui::themed_text(parent, theme.warn_48_text.style())
        {}
    };

}
