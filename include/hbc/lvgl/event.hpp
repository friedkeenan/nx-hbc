#pragma once

#include <hbc/common.hpp>

namespace hbc::lvgl {

    /* Forward declare. */
    struct object;

    struct event {
        lv_event_t *_event;

        inline const lv_event_t *underlying(this const event self) {
            return self._event;
        }

        inline lv_event_code_t code(this const event self) {
            return lv_event_get_code(self._event);
        }

        template<std::derived_from<lvgl::object> Target = lvgl::object>
        Target &current_target(this const event self) {
            const auto target = lv_event_get_current_target_obj(self._event);

            /*
                NOTE: We duplicate the logic for 'lvgl::object::from_underlying'
                here because currently 'lvgl::object' is an incomplete type.
            */
            return static_cast<Target &>(*static_cast<lvgl::object *>(
                lv_obj_get_user_data(target)
            ));
        }

        template<typename T>
        T *param(this const event self) {
            return static_cast<T *>(lv_event_get_param(self._event));
        }
    };

}
