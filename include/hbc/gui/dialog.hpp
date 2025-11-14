#pragma once

#include <hbc/common.hpp>

#include <hbc/lang/lang.hpp>

#include <hbc/lvgl/lvgl.hpp>

#include <hbc/gui/theme.hpp>
#include <hbc/gui/text.hpp>
#include <hbc/gui/cover.hpp>
#include <hbc/gui/image_button.hpp>

namespace hbc::gui {

    struct dialog_button : gui::image_button {
        static constexpr std::int32_t button_width  = decltype(gui::theme::tiny_button)::width;
        static constexpr std::int32_t button_height = decltype(gui::theme::tiny_button)::height;

        dialog_button(
            lvgl::parentable auto &parent,

            const gui::theme &theme,
            const lang::translated_view text
        )
        :
            gui::image_button(
                parent,

                theme.tiny_button.description(),
                theme.tiny_button_focus.description()
            )
        {
            static constexpr std::int32_t MaxTextWidth = button_width - 20;

            auto &label = this->add_child<gui::normal_28_text>(theme);
            label.set_text_translated(text);
            label.set_max_width(MaxTextWidth);

            label.center();
        }
    };

    namespace impl {

        template<std::derived_from<lvgl::label> TitleLabel>
        struct dialog : lvgl::image {
            using title_label = TitleLabel;

            static constexpr std::int32_t background_width  = decltype(gui::theme::dialog_background)::width;
            static constexpr std::int32_t background_height = decltype(gui::theme::dialog_background)::height;

            static constexpr std::int32_t vertical_offset   = 20;
            static constexpr std::int32_t horizontal_offset = 40;

            static constexpr std::int32_t content_width  = background_width  - 2 * horizontal_offset;
            static constexpr std::int32_t content_height = background_height - 2 * vertical_offset;

            /* NOTE: This uses 'std::type_identity_t' to enable implicit conversions. */
            template<typename Self>
            using button_click_callback = std::type_identity_t<void (*)(Self &)>;

            /* The starting index for extended children, added by derived classes. */
            static constexpr std::size_t extended_children_start = 3;

            static void _update_center_cover_callback(const lvgl::event event) {
                auto &self = event.current_target();

                const auto old_area = event.param<lv_area_t>();

                if (self.height() != lv_area_get_height(old_area)) {
                    self.parent<dialog>()._update_center_cover();
                }
            }

            dialog(
                lvgl::parentable auto &parent,

                const gui::theme &theme,
                const lang::translated_view title_text
            )
            :
                lvgl::image(parent)
            {
                this->set_description(theme.dialog_background.description());

                /*
                    Make clickable to swallow up the click event,
                    so it doesn't go to any objects behind us.
                */
                this->set_clickable(true);

                this->set_vertical_padding(vertical_offset);
                this->set_horizontal_padding(horizontal_offset);

                auto &title = this->add_child<title_label>(theme);
                title.set_text_translated(title_text);
                title.set_max_width(content_width);

                title.align(lvgl::alignment::top_center);

                auto &footer = this->add_child<gui::empty_object>();
                footer.set_size({LV_SIZE_CONTENT, LV_SIZE_CONTENT});
                footer.set_max_width(content_width);

                footer.align(lvgl::alignment::bottom_center);

                /* Center cover. */
                this->add_child<gui::empty_object>();
                this->_update_center_cover();

                footer.on_size_changed(_update_center_cover_callback);

                title.on_size_changed(_update_center_cover_callback);

                this->center();
            }

            title_label &title(this dialog &self) {
                return self.child<title_label>(0);
            }

            lvgl::object &footer(this dialog &self) {
                return self.child(1);
            }

            lvgl::object &center_cover(this dialog &self) {
                return self.child(2);
            }

            void _update_center_cover(this dialog &self) {
                auto &title  = self.title();
                auto &footer = self.footer();

                auto &center = self.center_cover();

                title.refresh_size();
                footer.refresh_size();

                const auto distance_from_top    = title.height()  + vertical_offset / 2;
                const auto distance_from_bottom = footer.height() + vertical_offset / 2;

                const auto center_height = (
                    content_height - distance_from_top - distance_from_bottom
                );

                center.set_size({content_width, center_height});

                center.set_pos({0, distance_from_top});

                center.refresh_pos();
                self.on_center_cover_updated();
            }

            virtual void on_center_cover_updated() {}

            template<std::derived_from<lvgl::object> Button, typename... Args, typename Self>
            requires (lvgl::child_constructible_from<Button, Args...>)
            Button &add_footer_button(
                this Self &self,

                button_click_callback<Self> callback,

                Args &&... args
            ) {
                auto &button = [&]() -> auto & {
                    lvgl::object &footer = self.footer();

                    if (footer.has_children()) {
                        auto &prev_button = footer.last_child();

                        auto &button = footer.add_child<Button>(std::forward<Args>(args)...);

                        button.align_to(prev_button, lvgl::alignment::out_right_center);

                        return button;
                    } else {
                        return footer.add_child<Button>(std::forward<Args>(args)...);
                    }
                }();

                /*
                    We manually manage adding this event so that
                    the intermediate footer object can remain an
                    implementation detail and something that the
                    caller does not need to deal with.
                */
                lv_obj_add_event_cb(button._object, [](lv_event_t *event) {
                    const auto button = lv_event_get_current_target_obj(event);
                    const auto footer = lv_obj_get_parent(button);
                    const auto dialog = lv_obj_get_parent(footer);

                    auto &self = static_cast<Self &>(
                        lvgl::object::from_underlying(dialog)
                    );

                    /* NOTE: This is okay, since we are doing a simple round-trip conversion. */
                    const auto real_callback = reinterpret_cast<decltype(callback)>(
                        lv_event_get_user_data(event)
                    );

                    real_callback(self);
                }, LV_EVENT_CLICKED, reinterpret_cast<void *>(callback));

                return button;
            }

            template<std::derived_from<lvgl::object> Object, typename... Args>
            requires (lvgl::child_constructible_from<Object, Args...>)
            Object &add_center_child(this dialog &self, Args &&... args) {
                return self.center_cover().template add_child<Object>(std::forward<Args>(args)...);
            }

            template<std::derived_from<lvgl::label> Label, typename... Args>
            requires (lvgl::child_constructible_from<Label, Args...>)
            Label &add_center_text(this dialog &self, Args &&... args) {
                auto &label = self.add_center_child<Label>(std::forward<Args>(args)...);

                label.set_max_width(content_width);
                label.set_long_mode(LV_LABEL_LONG_MODE_WRAP);

                label.set_text_align(LV_TEXT_ALIGN_CENTER);

                label.center();

                return label;
            }
        };

    }

    using dialog = impl::dialog<gui::normal_48_text>;

    struct error_dialog : impl::dialog<gui::warn_48_text> {
        error_dialog(
            lvgl::parentable auto &parent,

            const gui::theme &theme,
            const lang::language language,

            const lang::translated_view text
        )
        :
            impl::dialog<gui::warn_48_text>(
                parent,

                theme,

                lang::translations::dialog_title_error.translate(language)
            )
        {
            auto &center_text = this->add_center_text<gui::normal_28_text>(theme);

            center_text.set_text_translated(text);

            auto &ok_button = this->add_footer_button<gui::dialog_button>(
                [](error_dialog &self) {
                    self.remove_from_parent();
                },

                theme,
                lang::translations::dialog_button_ok.translate(language)
            );

            /* TODO: We really need to figure out keypad grouping. */
            ok_button.focus();
        }
    };

}
