#pragma once

#include <hbc/common.hpp>

#include <hbc/util/util.hpp>

#include <hbc/config/config.hpp>

#include <hbc/lvgl/lvgl.hpp>

namespace hbc::gui {

    constexpr inline std::size_t num_argb_components = 4;

    template<std::size_t N>
    requires (N != std::dynamic_extent && N % gui::num_argb_components == 0)
    void fill_with_error_color(std::span<std::uint8_t, N> color_data) {
        /*
            If we fail to load an image then we substitute it
            with an image of a solid, random reddish color.

            This is because we do not have the luxury of hard-failing.
            Since we are the homebrew menu, if we exited then we would
            just be loaded again and come back to the same place.

            Better to show *something* to the user for them to tell something's wrong.
        */

        const auto red_value   = static_cast<std::uint8_t>(lv_rand(0x80, 0xFF));
        const auto green_value = static_cast<std::uint8_t>(lv_rand(0x00, 0x40));
        const auto blue_value  = static_cast<std::uint8_t>(lv_rand(0x00, 0x40));

        for (auto color = color_data.begin(); color != color_data.end(); color += gui::num_argb_components) {
            /* NOTE: LVGL accepts the colors in the order of BGRA. */

            color[0] = blue_value;
            color[1] = green_value;
            color[2] = red_value;
            color[3] = 0xFF;
        }
    }

    template<util::fixed_string FileName, std::uint16_t Width, std::uint16_t Height>
    struct image_asset {
        HBC_NON_COPYABLE(image_asset);
        HBC_NON_MOVEABLE(image_asset);

        static constexpr auto width  = Width;
        static constexpr auto height = Height;

        static constexpr std::size_t data_size = gui::num_argb_components * Width * Height;

        /* Image descriptions store their data sizes as a 'std::uint32_t'. */
        static_assert(data_size <= std::numeric_limits<std::uint32_t>::max());

        lvgl::image_description _description;

        constexpr image_asset()
        :
            _description(lvgl::image_description::argb(Width, Height))
        {}

        constexpr const std::uint8_t *_data(this const image_asset &self) {
            return self._description.data().data();
        }

        constexpr std::uint8_t *_data(this image_asset &self) {
            /* NOTE: We know the data was not declared as const, so const-casting is acceptable here. */

            return const_cast<std::uint8_t *>(self._description.data().data());
        }

        constexpr void set_data(this image_asset &self, std::uint8_t *data) {
            self._description.set_data(std::span(data, data_size));
        }

        void _load_fallback(this image_asset &self) {
            gui::fill_with_error_color(
                std::span<std::uint8_t, data_size>(
                    self._data(), data_size
                )
            );
        }

        [[nodiscard]]
        bool _load_from_zip(this image_asset &self, util::zip_file &zip) {
            if (!zip.locate_file(FileName).has_value()) {
                return false;
            }

            return zip.open_current_file(
                [&](auto &file) {
                    const auto size = file.size();
                    if (!size.has_value() || size.value() != data_size) {
                        return false;
                    }

                    return file.read_into(
                        std::as_writable_bytes(
                            std::span(self._data(), data_size)
                        )
                    ).has_value();
                },

                [](int) {
                    return false;
                }
            );
        }

        void load(this image_asset &self, util::zip_file *base_theme, util::zip_file *user_theme) {
            if (user_theme != nullptr && self._load_from_zip(*user_theme)) {
                return;
            }

            if (base_theme != nullptr && self._load_from_zip(*base_theme)) {
                return;
            }

            self._load_fallback();
        }

        const lvgl::image_description &description(this const image_asset &self) {
            return self._description;
        }
    };

    /* Forward declare. */
    struct theme;

    namespace impl {

        template<typename MemberPtr>
        struct member_from_ptr;

        template<typename Member, typename Cls>
        struct member_from_ptr<Member (Cls:: *)> {
            using type = Member;
        };

        template<typename MemberPtr>
        using member_from_ptr_t = impl::member_from_ptr<MemberPtr>::type;

        template<auto... ImageAssetMembers>
        struct image_asset_data {
            static constexpr std::size_t cumulative_size = (
                0uz + ... + impl::member_from_ptr_t<decltype(ImageAssetMembers)>::data_size
            );

            template<auto MemberPtr>
            static consteval std::size_t asset_region_offset() {
                using Member = impl::member_from_ptr_t<decltype(MemberPtr)>;

                std::size_t offset = 0;

                /*
                    NOTE: Short circuiting will end this expression
                    once we come across the member pointer of interest.
                */
                (
                    (
                        offset += impl::member_from_ptr_t<decltype(ImageAssetMembers)>::data_size,

                        []() {
                            if constexpr (!std::same_as<decltype(MemberPtr), decltype(ImageAssetMembers)>) {
                                return false;
                            } else {
                                return MemberPtr == ImageAssetMembers;
                            }
                        }()
                    )

                    || ...
                );

                /* Subtract our own data size, which was eagerly added in the fold expression. */
                offset -= Member::data_size;

                return offset;
            }

            /* We store all the image data in one big buffer. */
            std::unique_ptr<std::uint8_t[]> _data = std::make_unique<std::uint8_t[]>(cumulative_size);

            explicit image_asset_data(gui::theme &theme) {
                (
                    this->set_asset_region<ImageAssetMembers>(theme),

                    ...
                );
            }

            template<auto MemberPtr>
            void set_asset_region(this image_asset_data &self, gui::theme &theme) {
                static constexpr auto Offset = asset_region_offset<MemberPtr>();

                (theme.*MemberPtr).set_data(self._data.get() + Offset);
            }

            static void load(gui::theme &theme, util::zip_file *base_theme, util::zip_file *user_theme) {
                (
                    (theme.*ImageAssetMembers).load(base_theme, user_theme),

                    ...
                );
            }
        };

        struct style_info {
            static constexpr util::zstring info_file_name = "info.toml";

            std::uint32_t normal_text_color         = 0xFFFFFF;
            std::uint32_t warn_text_color           = 0xFF0000;
            std::uint32_t darkened_background_color = 0x000000;

            static inline style_info load(util::zip_file *user_theme) {
                /*
                    NOTE: We don't bother checking the base theme for
                    style information, since it's defined in code here.
                */

                if (user_theme == nullptr) {
                    return style_info{};
                }

                if (!user_theme->locate_file(info_file_name).has_value()) {
                    return style_info{};
                }

                /* NOTE: Non-const for Glaze. */
                auto contents = user_theme->read_current_file_text();
                if (!contents.has_value()) {
                    return style_info{};
                }

                struct ParsedStyleInfo {
                    struct {
                        style_info style;
                    } hbc;
                };

                ParsedStyleInfo result = {};
                const auto error = glz::read<
                    glz::opts{
                        .format = glz::TOML,
                        .error_on_unknown_keys = false
                    }
                >(result, std::move(contents).value());

                if (error) {
                    /*
                        NOTE: We construct another 'style_info'
                        object because the one we were using
                        could be partially (and erroneously)
                        filled in by Glaze.
                    */
                    return style_info{};
                }

                return result.hbc.style;
            }
        };

        template<auto... FontStyleMembers>
        requires (sizeof...(FontStyleMembers) > 0)
        struct font_data {
            HBC_NON_COPYABLE(font_data);
            HBC_NON_MOVEABLE(font_data);

            static constexpr util::zstring font_file_name = "font.ttf";

            template<std::int32_t... UniqueSizes>
            static constexpr auto get_font_sizes(auto Head, auto... Tail) {
                /* Get all the unique sizes of our fonts, so we don't create unnecessary fonts. */

                using Member = impl::member_from_ptr_t<decltype(Head)>;

                if constexpr (((Member::font_size == UniqueSizes) || ...)) {
                    /* Don't add our size to the unique sizes, since it's already tracked. */

                    if constexpr (sizeof...(Tail) > 0) {
                        return get_font_sizes<UniqueSizes...>(Tail...);
                    } else {
                        return std::array{UniqueSizes...};
                    }
                } else {
                    /* Add our size to the unique sizes, since it isn't already tracked. */

                    if constexpr (sizeof...(Tail) > 0) {
                        return get_font_sizes<UniqueSizes..., Member::font_size>(Tail...);
                    } else {
                        return std::array{UniqueSizes..., Member::font_size};
                    }
                }
            }

            static constexpr auto font_sizes = get_font_sizes(FontStyleMembers...);

            util::unique_array<std::byte> _data;

            /*
                NOTE: We need to store the fonts as const because we
                store 'LV_FONT_DEFAULT' here, which will be const.
            */
            std::array<const lv_font_t *, font_sizes.size()> _fonts = {};

            constexpr font_data() = default;

            constexpr void _destroy_fonts(this font_data &self, const std::size_t end_index = font_sizes.size()) {
                for (const auto i : std::views::iota(0uz, end_index)) {
                    const auto font = self._fonts[i];
                    if (font == nullptr || font == LV_FONT_DEFAULT) {
                        continue;
                    }

                    lv_tiny_ttf_destroy(
                        /* NOTE: We know these fonts were constructed as non-const. */
                        const_cast<lv_font_t *>(
                            font
                        )
                    );
                }
            }

            constexpr ~font_data() {
                this->_destroy_fonts();
            }

            [[nodiscard]]
            bool _create_fonts(this font_data &self) {
                for (const auto i : std::views::iota(0uz, font_sizes.size())) {
                    const auto font = lv_tiny_ttf_create_data(self._data.data(), self._data.size(), font_sizes[i]);
                    if (font == nullptr) {
                        self._destroy_fonts(i);

                        return false;
                    }

                    self._fonts[i] = font;
                }

                return true;
            }

            [[nodiscard]]
            bool _load_from_zip(this font_data &self, util::zip_file &zip) {
                if (!zip.locate_file(font_file_name).has_value()) {
                    return false;
                }

                auto data = zip.read_current_file_bytes();
                if (!data.has_value()) {
                    return false;
                }

                self._data = std::move(data).value();

                return self._create_fonts();
            }

            void _load_fallback(this font_data &self) {
                std::ranges::fill(self._fonts, LV_FONT_DEFAULT);
            }

            void _load_fonts(this font_data &self, util::zip_file *base_theme, util::zip_file *user_theme) {
                self._destroy_fonts();

                if (user_theme != nullptr && self._load_from_zip(*user_theme)) {
                    return;
                }

                if (base_theme != nullptr && self._load_from_zip(*base_theme)) {
                    return;
                }

                self._load_fallback();
            }

            template<auto MemberPtr>
            void _load_style(this font_data &self, gui::theme &theme, const impl::style_info &info) {
                using Member = impl::member_from_ptr_t<decltype(MemberPtr)>;

                static constexpr std::size_t FontIndex = (
                    std::ranges::find(font_sizes, Member::font_size) - font_sizes.begin()
                );

                (theme.*MemberPtr).load(*self._fonts[FontIndex], info);
            }

            void load(
                this font_data &self,

                gui::theme &theme,
                const impl::style_info &info,

                util::zip_file *base_theme,
                util::zip_file *user_theme
            ) {
                self._load_fonts(base_theme, user_theme);

                (
                    self._load_style<FontStyleMembers>(theme, info),

                    ...
                );
            }
        };

        template<typename MemberPtr>
        concept style_color = requires(const MemberPtr &color, const impl::style_info &info) {
            { info.*color } -> std::convertible_to<std::uint32_t>;
        };
    }

    template<impl::style_color auto StyleColor, std::int32_t Size>
    struct font_style {
        static constexpr std::int32_t font_size = Size;

        lvgl::style _style;

        void load(this font_style &self, const lv_font_t &font, const impl::style_info &info) {
            self._style.set_text_font(font);
            self._style.set_text_color(lvgl::color::hex(info.*StyleColor));
        }

        const lvgl::style &style(this const font_style &self) {
            return self._style;
        }
    };


    struct theme {
        static constexpr util::zstring info_file_name = impl::style_info::info_file_name;

        static constexpr util::zstring icon_file_name = "icon.jpg";

        gui::image_asset<"app_button.bin",       648, 96> app_button;
        gui::image_asset<"app_button_focus.bin", 648, 96> app_button_focus;

        gui::image_asset<"next_page_button.bin",       96, 96> next_page_button;
        gui::image_asset<"next_page_button_focus.bin", 96, 96> next_page_button_focus;

        gui::image_asset<"previous_page_button.bin",       96, 96> previous_page_button;
        gui::image_asset<"previous_page_button_focus.bin", 96, 96> previous_page_button_focus;

        gui::image_asset<"background.bin", 1280, 720> background;

        gui::image_asset<"dialog_background.bin", 780, 540> dialog_background;

        gui::image_asset<"logo.bin", 381, 34> logo;

        /* TODO: Improve? Resize better at least. */
        gui::image_asset<"star_big.bin",   89, 86> star_big;
        gui::image_asset<"star_small.bin", 25, 24> star_small;

        gui::image_asset<"tiny_button.bin",       175, 72> tiny_button;
        gui::image_asset<"tiny_button_focus.bin", 175, 72> tiny_button_focus;

        /* TODO: Improve? */
        gui::image_asset<"unavailable_icon_big.bin",   256, 256> unavailable_icon_big;
        gui::image_asset<"unavailable_icon_small.bin", 72,  72>  unavailable_icon_small;

        gui::font_style<&impl::style_info::normal_text_color, 16> normal_16_text;
        gui::font_style<&impl::style_info::normal_text_color, 28> normal_28_text;
        gui::font_style<&impl::style_info::normal_text_color, 48> normal_48_text;

        gui::font_style<&impl::style_info::warn_text_color, 48> warn_48_text;

        /* TODO: Generate with reflection. */
        using image_asset_data = impl::image_asset_data<
            &theme::app_button,
            &theme::app_button_focus,

            &theme::next_page_button,
            &theme::next_page_button_focus,

            &theme::previous_page_button,
            &theme::previous_page_button_focus,

            &theme::background,

            &theme::dialog_background,

            &theme::logo,

            &theme::star_big,
            &theme::star_small,

            &theme::tiny_button,
            &theme::tiny_button_focus,

            &theme::unavailable_icon_big,
            &theme::unavailable_icon_small
        >;

        /* TODO: Generate with reflection. */
        using font_data = impl::font_data<
            &theme::normal_16_text,
            &theme::normal_28_text,
            &theme::normal_48_text,

            &theme::warn_48_text
        >;

        impl::style_info _style_info;

        image_asset_data _image_asset_data;
        font_data        _font_data;

        inline theme() : _image_asset_data(*this) {
            this->reload();
        }

        inline void _load_images(this theme &self, util::zip_file *base_theme, util::zip_file *user_theme) {
            self._image_asset_data.load(self, base_theme, user_theme);
        }

        inline void _load_styles(this theme &self, util::zip_file *base_theme, util::zip_file *user_theme) {
            self._style_info = impl::style_info::load(user_theme);

            self._font_data.load(self, self._style_info, base_theme, user_theme);
        }

        inline void reload(this theme &self) {
            #if defined(__SWITCH__)

            /* NOTE: We don't need initialization to succeed here. */
            auto _ = util::romfs_guard::init();

            #endif

            auto base_theme = util::zip_file::open(config::paths::base_theme);
            auto user_theme = util::zip_file::open(config::paths::user_theme);

            /* TODO: Optional references. */
            const auto base_theme_ptr = base_theme
                .transform([](auto &theme) {
                    return &theme;
                })
                .value_or(nullptr);

            const auto user_theme_ptr = user_theme
                .transform([](auto &theme) {
                    return &theme;
                })
                .value_or(nullptr);

            self._load_images(base_theme_ptr, user_theme_ptr);
            self._load_styles(base_theme_ptr, user_theme_ptr);
        }

        /* TODO: Generate these methods with reflection. */

        inline lvgl::color normal_text_color(this const theme &self) {
            return lvgl::color::hex(self._style_info.normal_text_color);
        }

        inline lvgl::color warn_text_color(this const theme &self) {
            return lvgl::color::hex(self._style_info.warn_text_color);
        }

        inline lvgl::color darkened_background_color(this const theme &self) {
            return lvgl::color::hex(self._style_info.darkened_background_color);
        }
    };

}
