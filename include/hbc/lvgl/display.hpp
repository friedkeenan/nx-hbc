#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/init.hpp>
#include <hbc/lvgl/object.hpp>

namespace hbc::lvgl {

    template<std::derived_from<lvgl::object> Screen>
    struct display {
        HBC_NON_COPYABLE(display);
        HBC_NON_MOVEABLE(display);

        lv_display_t *_display;

        std::optional<Screen> _screen;

        static display &from_underlying(lv_display_t *underlying) {
            /*
                NOTE: We don't use the "driver data" of the display
                to store ourselves, since on PC that will conflict
                with the SDL information that LVGL stores there.
            */

            return *static_cast<display *>(
                lv_display_get_user_data(underlying)
            );
        }

        #if defined(__SWITCH__)

        Framebuffer _framebuffer;

        display(const std::int32_t width, const std::int32_t height)
        :
            _display(lv_display_create(width, height))
        {
            const auto window = nwindowGetDefault();

            LV_ASSERT(R_SUCCEEDED(
                framebufferCreate(&this->_framebuffer, window, width, height, PIXEL_FORMAT_BGRA_8888, 2)
            ));

            LV_ASSERT(R_SUCCEEDED(
                framebufferMakeLinear(&this->_framebuffer)
            ));

            this->set_buffer();

            lv_display_set_user_data(this->_display, this);

            lv_display_set_flush_cb(this->_display, [](lv_display_t *display, const lv_area_t *, std::uint8_t *) {
                auto &self = from_underlying(display);

                framebufferEnd(&self._framebuffer);

                self.set_buffer();

                lv_display_flush_ready(self._display);
            });
        }

        void set_buffer(this display &self) {
            std::uint32_t stride;
            const auto buffer = framebufferBegin(&self._framebuffer, &stride);

            const auto buffer_size = stride * self.height();

            lv_display_set_buffers_with_stride(
                self._display,

                buffer,
                nullptr,
                buffer_size,
                stride,

                LV_DISPLAY_RENDER_MODE_DIRECT
            );
        }

        #else

        display(const std::int32_t width, const std::int32_t height)
        :
            _display(lv_sdl_window_create(width, height))
        {
            lv_sdl_window_set_resizeable(this->_display, false);

            lv_display_set_user_data(this->_display, this);
        }

        #endif

        ~display() {
            if (impl::already_destroyed()) {
                return;
            }

            if (this->_screen.has_value()) {
                /*
                    The liftime of our screen's underlying
                    object is managed by our display.
                */

                /* Destroy all the children of our screen. */
                this->_screen->clear();

                (void) this->_screen->_release();
            }

            lv_display_delete(this->_display);

            #if defined(__SWITCH__)

            framebufferClose(&this->_framebuffer);

            #endif
        }

        std::int32_t width(this const display &self) {
            return lv_display_get_horizontal_resolution(self._display);
        }

        std::int32_t height(this const display &self) {
            return lv_display_get_vertical_resolution(self._display);
        }

        template<typename... Args>
        requires (std::constructible_from<Screen, const impl::screen_t &, Args...>)
        Screen &load_screen(this display &self, Args &&... args) {
            auto &screen = self._screen.emplace(
                impl::screen,
                std::forward<Args>(args)...
            );

            lv_screen_load(screen._object);

            return screen;
        }

        inline void signal_theme_reloaded(this display &self, const gui::theme &theme) {
            if (!self._screen.has_value()) {
                return;
            }

            self._screen->invalidate();

            self._screen->signal_theme_reloaded(theme);
        }
    };

}
