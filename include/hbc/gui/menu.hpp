#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/lvgl.hpp>

#include <hbc/gui/theme.hpp>
#include <hbc/gui/app.hpp>
#include <hbc/gui/text.hpp>
#include <hbc/gui/cover.hpp>
#include <hbc/gui/image_button.hpp>
#include <hbc/gui/dialog.hpp>

namespace hbc::gui {

    /* Forward declare. */
    struct interface;

    /* NOTE: We use a template to get around errors about 'gui::interface' being incomplete. */
    template<std::same_as<gui::interface> Interface = gui::interface>
    struct menu : lvgl::image {
        static constexpr std::size_t apps_per_page = 5;

        struct app_button : gui::image_button {
            static constexpr std::int32_t button_width  = decltype(gui::theme::app_button)::width;
            static constexpr std::int32_t button_height = decltype(gui::theme::app_button)::height;

            gui::app &app;

            app_button(
                lvgl::parentable auto &parent,

                const gui::theme &theme,
                gui::app &app
            )
            :
                gui::image_button(
                    parent,

                    theme.app_button.description(),
                    theme.app_button_focus.description()
                ),

                app(app)
            {
                static constexpr std::int32_t Offset = (button_height - gui::app::small_icon_height) / 2;

                static constexpr std::int32_t NameX = (
                    Offset +
                    gui::app::small_icon_width +
                    Offset
                );

                static constexpr std::int32_t MaxTextWidth = (
                    button_width - Offset -

                    NameX
                );

                auto &author = this->add_child<gui::normal_16_text>(theme);
                author.set_text_translated(this->app.meta.author);
                author.set_max_width(MaxTextWidth);

                author.align(lvgl::alignment::bottom_right, {-Offset, -Offset});

                auto &version = this->add_child<gui::normal_16_text>(theme);
                version.set_text_translated(this->app.meta.version);
                version.set_max_width(MaxTextWidth);

                version.align(lvgl::alignment::top_right, {-Offset, Offset});

                auto &icon = this->add_child<lvgl::image>();
                icon.set_description(this->app.small_icon(theme));

                icon.align(lvgl::alignment::left_center, {Offset, 0});

                if (this->app.starred) {
                    static constexpr lvgl::vector_2d<std::int32_t> StarOffset = {
                        -static_cast<std::int32_t>(decltype(gui::theme::star_small)::width)  / 2,
                        -static_cast<std::int32_t>(decltype(gui::theme::star_small)::height) / 2
                    };

                    auto &star = this->add_child<lvgl::image>();
                    star.set_description(theme.star_small.description());

                    star.align_to(icon, lvgl::alignment::top_left, StarOffset);
                }

                auto &name = this->add_child<gui::normal_28_text>(theme);
                name.set_text_translated(this->app.meta.name);
                name.set_max_width(MaxTextWidth);

                static constexpr lvgl::vector_2d<std::int32_t> NameOffset = {
                    gui::app::small_icon_width + 2 * Offset,

                    0
                };

                /*
                    NOTE: We don't 'align_to' the icon so that the name
                    repositions itself properly when the font is changed.
                */
                name.align(lvgl::alignment::left_center, NameOffset);

                this->on_clicked([](const lvgl::event event) {
                    app_button &self = event.current_target<app_button>();

                    menu &root = self.parent<menu>();

                    auto &darkened_background = root.add_child<gui::darkened_background>(root.interface.theme);

                    darkened_background.template add_child<app_dialog>(
                        root.interface.theme,
                        root.interface.language(),
                        self.app
                    );
                });
            }
        };

        struct previous_page_button : gui::image_button {
            static constexpr std::int32_t button_width  = decltype(gui::theme::previous_page_button)::width;
            static constexpr std::int32_t button_height = decltype(gui::theme::previous_page_button)::height;

            previous_page_button(lvgl::parentable auto &parent, const gui::theme &theme)
            :
                gui::image_button(
                    parent,

                    theme.previous_page_button.description(),
                    theme.previous_page_button_focus.description()
                )
            {
                this->on_clicked([](const lvgl::event event) {
                    auto &self = event.current_target();

                    auto &root = self.parent<menu>();

                    root.decrement_page();
                });
            }
        };

        struct next_page_button : gui::image_button {
            static constexpr std::int32_t button_width  = decltype(gui::theme::next_page_button)::width;
            static constexpr std::int32_t button_height = decltype(gui::theme::next_page_button)::height;

            next_page_button(lvgl::parentable auto &parent, const gui::theme &theme)
            :
                gui::image_button(
                    parent,

                    theme.next_page_button.description(),
                    theme.next_page_button_focus.description()
                )
            {
                this->on_clicked([](const lvgl::event event) {
                    auto &self = event.current_target();

                    auto &root = self.parent<menu>();

                    root.increment_page();
                });
            }
        };

        struct app_delete_dialog : gui::dialog {
            app_delete_dialog(
                lvgl::parentable auto &parent,

                const gui::theme &theme,
                const lang::language language
            )
            :
                gui::dialog(
                    parent,

                    theme,
                    lang::translations::dialog_title_confirmation.translate(language)
                )
            {
                auto &text = this->add_center_text<gui::normal_28_text>(theme);

                text.set_text_translated(
                    lang::translations::dialog_text_app_deletion.translate(language)
                );

                /* TODO: Better-sized buttons. */
                this->add_footer_button<gui::dialog_button>(
                    [](app_delete_dialog &self) {
                        auto &parent = self.parent<app_dialog>();
                        auto &root   = parent.template nth_parent<menu>(1);

                        if (!parent.app.remove()) {
                            const auto language = root.interface.language();

                            self.add_child<gui::error_dialog>(
                                root.interface.theme,
                                language,

                                lang::translations::dialog_error_delete.translate(language)
                            );

                            return;
                        }

                        root.reset_on_same_page();
                    },

                    theme,
                    lang::translations::dialog_button_yes.translate(language)
                );

                auto &no_button = this->add_footer_button<gui::dialog_button>(
                    [](app_delete_dialog &self) {
                        self.remove_from_parent();
                    },

                    theme,
                    lang::translations::dialog_button_no.translate(language)
                );

                no_button.focus();
            }
        };

        struct app_dialog : gui::dialog {
            gui::app &app;

            app_dialog(
                lvgl::parentable auto &parent,

                const gui::theme &theme,
                const lang::language language,
                gui::app &app
            )
            :
                gui::dialog(
                    parent,

                    theme,
                    app.meta.name
                ),

                app(app)
            {
                auto &icon = this->add_center_child<lvgl::image>();
                icon.set_description(this->app.big_icon(theme));

                icon.align(lvgl::alignment::top_left);

                if (this->app.starred) {
                    /* NOTE: The star will get aligned properly later. */

                    auto &star = this->add_child<lvgl::image>();
                    star.set_description(theme.star_big.description());
                }

                static constexpr lvgl::vector_2d<std::int32_t> DescriptionPos = {
                    gui::app::big_icon_width + horizontal_offset,

                    vertical_offset
                };

                static constexpr std::int32_t MaxDescriptionWidth = content_width - DescriptionPos.x;

                auto &version = this->add_center_child<gui::normal_28_text>(theme);
                version.set_text_translated(
                    lang::translations::app_version_description.translate(
                        language,

                        this->app.meta.version
                    )
                );

                version.set_max_width(MaxDescriptionWidth);

                version.align(lvgl::alignment::top_left, DescriptionPos);

                /* NOTE: The author label will get aligned properly later. */
                auto &author = this->add_center_child<gui::normal_28_text>(theme);
                author.set_text_translated(
                    lang::translations::app_author_description.translate(
                        language,

                        this->app.meta.author
                    )
                );

                author.set_max_width(MaxDescriptionWidth);

                /*
                    NOTE: We order the buttons as load, star, delete, then back.

                    The original HBC orders them as delete, load, then back.
                */

                /* Load button. */
                auto &load_button = this->add_footer_button<gui::dialog_button>(
                    [](app_dialog &self) {
                        auto &root = self.template nth_parent<menu>(1);

                        if (!self.app.load(root.interface)) {
                            const auto language = root.interface.language();

                            self.add_child<gui::error_dialog>(
                                root.interface.theme,
                                language,

                                lang::translations::dialog_error_load.translate(language)
                            );
                        }
                    },

                    theme,
                    lang::translations::dialog_button_load.translate(language)
                );

                load_button.focus();

                /* Star button. */
                this->add_footer_button<gui::dialog_button>(
                    [](app_dialog &self) {
                        auto &root = self.template nth_parent<menu>(1);

                        if (!self.app.toggle_starred()) {
                            const auto language = root.interface.language();

                            self.add_child<gui::error_dialog>(
                                root.interface.theme,
                                language,

                                [&]() {
                                    if (self.app.starred) {
                                        return lang::translations::dialog_error_unstar.translate(language);
                                    }

                                    return lang::translations::dialog_error_star.translate(language);
                                }()
                            );

                            return;
                        }

                        root.reset_focused_on(self.app);
                    },

                    theme,

                    [&]() {
                        if (this->app.starred) {
                            return lang::translations::dialog_button_unstar.translate(language);
                        }

                        return lang::translations::dialog_button_star.translate(language);
                    }()
                );

                /* Delete button. */
                this->add_footer_button<gui::dialog_button>(
                    [](app_dialog &self) {
                        auto &root = self.template nth_parent<menu>(1);

                        self.add_child<app_delete_dialog>(
                            root.interface.theme,
                            root.interface.language()
                        );
                    },

                    theme,
                    lang::translations::dialog_button_delete.translate(language)
                );

                /* Back button. */
                this->add_footer_button<gui::dialog_button>(
                    [](app_dialog &self) {
                        self.remove_branch_from_nth_parent(1);
                    },

                    theme,
                    lang::translations::dialog_button_back.translate(language)
                );

                this->on_center_cover_updated();
            }

            bool has_star(this const app_dialog &self) {
                return self.num_children() > extended_children_start;
            }

            lvgl::object &star_object(this app_dialog &self) {
                return self.child(extended_children_start);
            }

            void on_center_cover_updated() override {
                auto &center = this->center_cover();

                auto &author = center.child(2);

                auto &version = center.child(1);
                version.refresh_size();

                author.align_to(version, lvgl::alignment::out_bottom_left, {0, vertical_offset});

                if (!this->has_star()) {
                    return;
                }

                auto &star = this->star_object();

                static constexpr lvgl::vector_2d<std::int32_t> StarOffset = {
                    -static_cast<std::int32_t>(decltype(gui::theme::star_big)::width)  / 2,
                    -static_cast<std::int32_t>(decltype(gui::theme::star_big)::height) / 2
                };

                const auto &icon = center.first_child();
                star.align_to(icon, lvgl::alignment::top_left, StarOffset);
            }
        };

        lvgl::group _group;

        Interface &interface;

        std::vector<gui::app> apps;
        std::size_t current_page = 0;

        menu(lvgl::parentable auto &parent, Interface &interface)
        :
            lvgl::image(parent),

            interface(interface),
            apps(gui::app::get_sorted_apps(interface.language()))
        {
            this->set_description(this->interface.theme.background.description());

            this->setup_background_elements();

            this->draw_apps_list();

            /* TODO: Dialog for last load result for Switch. */
        }

        void setup_background_elements(this menu &self) {
            /*
                An object to better organize the children hierarchy and
                separate background elements from the app buttons and such.
            */
            auto &background = self.add_child<gui::cover>();

            static constexpr lvgl::vector_2d<std::int32_t> LogoOffset = {10, -32};

            auto &logo = background.add_child<lvgl::image>();
            logo.set_description(self.interface.theme.logo.description());
            logo.align(lvgl::alignment::bottom_left, LogoOffset);
        }

        std::span<gui::app> visible_apps(this menu &self) {
            const auto start_index = self.current_page * apps_per_page;
            const auto size = std::min(apps_per_page, self.apps.size() - start_index);

            return std::span(self.apps.begin() + start_index, size);
        }

        static constexpr std::int32_t app_button_y(const std::size_t index) {
            static constexpr std::int32_t FirstButtonY = (
                (Interface::display_height - apps_per_page * app_button::button_height) / 2
            );

            return (
                FirstButtonY +

                app_button::button_height *
                static_cast<std::int32_t>(index)
            );
        }

        void reset_focused_on(this menu &self, const gui::app &app) {
            const auto path = app.path();

            self.apps = gui::app::get_sorted_apps(self.interface.language());

            const auto it = std::ranges::find(self.apps, path, &gui::app::path);

            std::size_t focused_index;
            if (it == self.apps.end()) {
                self.current_page = 0;
                focused_index     = 0;
            } else {
                const auto app_index = it - self.apps.begin();

                self.current_page = app_index / apps_per_page;
                focused_index     = app_index % apps_per_page;
            }

            self.draw_apps_list(focused_index);
        }

        std::size_t last_page(this const menu &self) {
            return std::sub_sat(self.apps.size(), 1uz) / apps_per_page;
        }

        void reset_on_same_page(this menu &self) {
            self.apps = gui::app::get_sorted_apps(self.interface.language());

            self.current_page = std::min(self.current_page, self.last_page());

            self.draw_apps_list();
        }

        enum class _page_button_focus : std::uint8_t {
            none,
            previous,
            next,
        };

        /* TODO: Animation. Would be disabled with a setting. */
        void increment_page(this menu &self) {
            self.current_page = std::clamp(self.current_page + 1, 0uz, self.last_page());

            self._draw_apps_list();
            self._draw_page_buttons<_page_button_focus::next>();
        }

        void decrement_page(this menu &self) {
            self.current_page = std::clamp(self.current_page - 1, 0uz, self.last_page());

            self._draw_apps_list();
            self._draw_page_buttons<_page_button_focus::previous>();
        }

        template<_page_button_focus Focus>
        void _draw_page_buttons(this menu &self) {
            static constexpr std::int32_t ButtonOffset = 40;

            if (self.current_page > 0) {
                auto &button = self.add_child<previous_page_button>(
                    self.interface.theme
                );

                button.align(lvgl::alignment::left_center, {ButtonOffset, 0});

                if constexpr (Focus == _page_button_focus::previous) {
                    button.focus();
                }
            }

            if (self.current_page < self.last_page()) {
                auto &button = self.add_child<next_page_button>(
                    self.interface.theme
                );

                button.align(lvgl::alignment::right_center, {-ButtonOffset, 0});

                if constexpr (Focus == _page_button_focus::next) {
                    button.focus();
                }
            }
        }

        void _draw_apps_list(this menu &self, const std::size_t focused_index = 0) {
            self.clear_after_index(0);

            if (self.apps.empty()) {
                gui::dialog &dialog = self.add_child<gui::dialog>(
                    self.interface.theme,

                    lang::translations::dialog_title_no_apps.translate(self.interface.language())
                );

                auto &text = dialog.add_center_text<gui::normal_28_text>(self.interface.theme);

                text.set_text_translated(
                    lang::translations::dialog_text_no_apps.translate(self.interface.language())
                );

                return;
            }

            for (const auto [i, app] : self.visible_apps() | std::views::enumerate) {
                auto &button = self.add_child<app_button>(self.interface.theme, app);

                button.set_pos({
                    (Interface::display_width - app_button::button_width) / 2,

                    app_button_y(i),
                });

                if (std::cmp_equal(i, focused_index)) {
                    button.focus();
                }
            }
        }

        void draw_apps_list(this menu &self, const std::size_t focused_index = 0) {
            self._draw_apps_list(focused_index);

            self._draw_page_buttons<_page_button_focus::none>();
        }
    };

}
