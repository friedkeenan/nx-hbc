#pragma once

#include <hbc/common.hpp>

#include <hbc/lang/lang.hpp>

#include <hbc/config/config.hpp>

#include <hbc/lvgl/lvgl.hpp>

#include <hbc/gui/theme.hpp>
#include <hbc/gui/menu.hpp>

namespace hbc::gui {

    struct interface {
        static constexpr std::int32_t display_width  = decltype(gui::theme::background)::width;
        static constexpr std::int32_t display_height = decltype(gui::theme::background)::height;

        lvgl::library_session _session;

        lvgl::display<gui::menu<>> display;

        lvgl::pointer_input _touch;

        /* TODO: Load here. */
        config::settings settings;

        gui::theme theme;

        /* NOTE: Atomic so it can be safely modified from other threads. */
        std::atomic<bool> _is_quitting = false;

        /* TODO: Move into keypad input. */
        #if defined(__SWITCH__)

        PadState pad;

        #endif

        inline interface() : display(display_width, display_height) {
            #if defined(__SWITCH__)

            padConfigureInput(1, HidNpadStyleSet_NpadStandard);

            padInitializeDefault(&this->pad);

            #endif

            this->display.load_screen(*this);
        }

        inline lang::language language(this const interface &self) {
            return self.settings.language;
        }

        inline void reload_theme(this interface &self) {
            self.theme.reload();

            lv_obj_report_style_change(nullptr);

            self.display.signal_theme_reloaded(self.theme);
        }

        inline void quit(this interface &self) {
            self._is_quitting.store(true);
        }

        inline bool _should_continue(this interface &self) {
            #if defined(__SWITCH__)

            if (!appletMainLoop()) {
                return false;
            }

            padUpdate(&self.pad);

            const auto keys_down = padGetButtonsDown(&self.pad);

            if ((keys_down & HidNpadButton_Plus) != 0) {
                return false;
            }

            return true;

            #else

            (void) self;

            /* The SDL display deinitializes SDL when it gets closed. */
            return SDL_WasInit(SDL_INIT_VIDEO);

            #endif
        }

        inline bool should_continue(this interface &self) {
            return self._should_continue() && !self._is_quitting.load();
        }

        inline void run(this interface &self) {
            while (self.should_continue()) {
                lv_timer_periodic_handler();
            }
        }
    };

}
