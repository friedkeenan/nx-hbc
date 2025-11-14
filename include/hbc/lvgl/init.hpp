#pragma once

#include <hbc/common.hpp>

namespace hbc::lvgl {

    namespace impl {

        inline std::uint32_t elapsed_milliseonds() {
            #if defined(__SWITCH__)

            return static_cast<std::uint32_t>(
                (armGetSystemTick() * 1000) / armGetSystemTickFreq()
            );

            #else

            return SDL_GetTicks();

            #endif
        }

        constexpr bool already_destroyed() {
            #if defined(__SWITCH__)

            return false;

            #else

            /* The SDL display handles deinitializing all of LVGL. */
            return !SDL_WasInit(SDL_INIT_VIDEO);

            #endif
        }

    }

    struct library_session {
        HBC_NON_COPYABLE(library_session);
        HBC_NON_MOVEABLE(library_session);

        inline library_session() {
            lv_init();

            lv_tick_set_cb(impl::elapsed_milliseonds);
        }

        inline ~library_session() {
            if (impl::already_destroyed()) {
                return;
            }

            lv_deinit();
        }
    };

}
