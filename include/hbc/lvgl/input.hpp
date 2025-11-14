#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/init.hpp>
#include <hbc/lvgl/object.hpp>

namespace hbc::lvgl {

    struct pointer_input {
        HBC_NON_COPYABLE(pointer_input);
        HBC_NON_MOVEABLE(pointer_input);

        lv_indev_t *_input;

        #if defined(__SWITCH__)

        inline pointer_input() : _input(lv_indev_create()) {
            lv_indev_set_type(this->_input, LV_INDEV_TYPE_POINTER);

            hidInitializeTouchScreen();

            lv_indev_set_read_cb(this->_input, [](lv_indev_t *, lv_indev_data_t *data) {
                HidTouchScreenState state;

                if (hidGetTouchScreenStates(&state, 1) < 1 || state.count <= 0) {
                    data->state = LV_INDEV_STATE_RELEASED;

                    data->point.x = -1;
                    data->point.y = -1;

                    return;
                }

                data->state = LV_INDEV_STATE_PRESSED;

                data->point.x = state.touches[0].x;
                data->point.y = state.touches[0].y;
            });
        }

        #else

        inline pointer_input() : _input(lv_sdl_mouse_create()) {}

        #endif

        inline ~pointer_input() {
            if (impl::already_destroyed()) {
                return;
            }

            lv_indev_delete(this->_input);
        }
    };

    struct group {
        HBC_NON_COPYABLE(group);
        HBC_NON_MOVEABLE(group);

        lv_group_t *_group;

        inline group() : _group(lv_group_create()) {}

        inline ~group() {
            if (impl::already_destroyed()) {
                return;
            }

            lv_group_delete(this->_group);
        }

        inline void add(this group &self, lvgl::object &object) {
            lv_group_add_obj(self._group, object.underlying());
        }
    };

}
