#pragma once

#include <hbc/common.hpp>

namespace hbc::gui {

    struct empty_object : lvgl::object {
        empty_object(lvgl::parentable auto &parent, const std::uint8_t opacity = 0)
        :
            lvgl::object(parent)
        {
            this->set_background_opacity(opacity);

            this->set_radius(0);
            this->set_border_width(0);
            this->set_padding(0);
        }
    };

    /* A simple object that covers its parent. */
    struct cover : gui::empty_object {
        inline cover(lvgl::object &parent, const std::uint8_t opacity = 0)
        :
            gui::empty_object(parent, opacity)
        {
            this->set_size(parent.size());
        }
    };

    struct darkened_background : gui::cover {
        static constexpr std::uint8_t opacity = 64;

        inline darkened_background(lvgl::object &parent, const gui::theme &theme) : gui::cover(parent, opacity) {
            this->set_background_color(theme.darkened_background_color());

            this->on_clicked([](const lvgl::event event) {
                auto &self = event.current_target();

                self.remove_from_parent();
            });
        }

        void on_theme_reload(const gui::theme &theme) override {
            this->set_background_color(theme.darkened_background_color());
        }
    };

}
