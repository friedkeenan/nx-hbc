#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/lvgl.hpp>

namespace hbc::gui {

    /* NOTE: This is different than LVGL's image button class. */
    struct image_button : lvgl::image {
        const lvgl::image_description &_inactive;
        const lvgl::image_description &_active;

        static void _set_active_callback(const lvgl::event event) {
            auto &self = event.current_target<image_button>();

            if (self.is_pressed() || self.is_hovered()) {
                self.set_active();
            } else {
                self.set_inactive();
            }
        }

        image_button(
            lvgl::parentable auto &parent,

            const lvgl::image_description &inactive,
            const lvgl::image_description &active
        )
        :
            lvgl::image(parent),

            _inactive(inactive),
            _active(active)
        {
            /* Images by default aren't clickable. */
            this->set_clickable(true);

            this->set_inactive();

            this->on_pressed(_set_active_callback);
            this->on_released(_set_active_callback);

            this->on_hover_over(_set_active_callback);
            this->on_hover_leave(_set_active_callback);
        }

        void set_inactive(this image_button &self) {
            self.set_description(self._inactive);
        }

        void set_active(this image_button &self) {
            self.set_description(self._active);
        }
    };

}
