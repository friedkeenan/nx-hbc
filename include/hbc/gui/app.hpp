#pragma once

#include <hbc/common.hpp>

#include <hbc/util/util.hpp>

#include <hbc/lang/lang.hpp>

#include <hbc/config/config.hpp>

#include <hbc/lvgl/lvgl.hpp>

#include <hbc/gui/theme.hpp>

/* A hack for us to allow "nullable" strings in TOML, where if they are not specified they are null. */
template<>
struct glz::from<glz::TOML, std::optional<std::string>> {
    template<auto Opts>
    static constexpr void op(auto &&value, auto &&ctx, auto &&it, auto &&end) noexcept {
        value.emplace();

        glz::from<glz::TOML, std::string>::template op<Opts>(*value, ctx, it, end);
    }
};

namespace hbc::gui {

    namespace impl {

        struct unprocessed_metadata {
            std::optional<std::string> name;
            std::optional<std::string> author;
            std::optional<std::string> version;

            void replace_newlines(this unprocessed_metadata &self) {
                static constexpr char Newline     = '\n';
                static constexpr char Replacement = ' ';

                if (self.name.has_value()) {
                    std::ranges::replace(self.name.value(), Newline, Replacement);
                }

                if (self.author.has_value()) {
                    std::ranges::replace(self.author.value(), Newline, Replacement);
                }

                if (self.version.has_value()) {
                    std::ranges::replace(self.version.value(), Newline, Replacement);
                }
            }
        };

        struct theme_app {
            static constexpr std::string_view extension = ".nx.hbc";

            std::string _path;

            constexpr const std::string &path(this const theme_app &self) {
                return self._path;
            }

            inline impl::unprocessed_metadata read_metadata(this const theme_app &self) {
                auto zip = util::zip_file::open(self._path);
                if (!zip.has_value()) {
                    return {};
                }

                if (!zip->locate_file(gui::theme::info_file_name).has_value()) {
                    return {};
                }

                /* NOTE: Non-const for Glaze. */
                auto contents = zip->read_current_file_text();
                if (!contents.has_value()) {
                    return {};
                }

                struct ParsedMetaInfo {
                    struct {
                        impl::unprocessed_metadata meta;
                    } hbc;
                };

                ParsedMetaInfo result;
                const auto error = glz::read<
                    glz::opts{
                        .format = glz::TOML,
                        .error_on_unknown_keys = false
                    }
                >(result, std::move(contents).value());

                if (error) {
                    return {};
                }

                return result.hbc.meta;
            }

            inline std::optional<util::unique_array<std::byte>> read_icon(this const theme_app &self) {
                auto zip = util::zip_file::open(self._path);
                if (!zip.has_value()) {
                    return std::nullopt;
                }

                if (!zip->locate_file(gui::theme::icon_file_name).has_value()) {
                    return std::nullopt;
                }

                auto contents = zip->read_current_file_bytes();
                if (!contents.has_value()) {
                    return std::nullopt;
                }

                return std::move(contents).value();
            }

            inline bool load(this const theme_app &self, auto &interface) {
                if (!util::directory::ensure_exists(config::paths::config_directory).has_value()) {
                    return false;
                }

                if (!util::file::copy_to(self._path, config::paths::user_theme).has_value()) {
                    return false;
                }

                interface.reload_theme();

                return true;
            }
        };

        struct executable_app {
            static constexpr std::string_view extension = ".nro";

            struct asset_section {
                template<std::size_t AssetIndex>
                [[nodiscard]]
                static inline std::optional<asset_section> read(util::file &file) {
                    static constexpr long NroMagicOffset = 0x10;

                    if (!file.seek_from_start(NroMagicOffset).has_value()) {
                        return std::nullopt;
                    }

                    const auto nro_magic = file.bit_read<std::array<char, 4>>();
                    if (!nro_magic.has_value()) {
                        return std::nullopt;
                    }

                    static constexpr auto NroMagic = std::array{
                        'N', 'R', 'O', '0'
                    };

                    if (nro_magic.value() != NroMagic) {
                        return std::nullopt;
                    }

                    static constexpr long NroSizeOffset = 0x18;

                    if (!file.seek_from_start(NroSizeOffset).has_value()) {
                        return std::nullopt;
                    }

                    const auto nro_size = file.read_little_endian<std::uint32_t>();
                    if (!nro_size.has_value()) {
                        return std::nullopt;
                    }

                    if (!file.seek_from_start(nro_size.value()).has_value()) {
                        return std::nullopt;
                    }

                    const auto assets_magic = file.bit_read<std::array<char, 4>>();
                    if (!assets_magic.has_value()) {
                        return std::nullopt;
                    }

                    static constexpr auto AssetsMagic = std::array{
                        'A', 'S', 'E', 'T'
                    };

                    if (assets_magic.value() != AssetsMagic) {
                        return std::nullopt;
                    }

                    /* If we don't know the format version, then we give up. */
                    const auto assets_format_version = file.read_little_endian<std::uint32_t>();
                    if (!assets_format_version.has_value()) {
                        return std::nullopt;
                    }

                    static constexpr std::uint32_t MaxSupportedFormatVersion = 0;

                    if (assets_format_version.value() > MaxSupportedFormatVersion) {
                        return std::nullopt;
                    }

                    if (!file.seek(0x10 * AssetIndex).has_value()) {
                        return std::nullopt;
                    }

                    const auto section_offset = file.read_little_endian<std::uint64_t>();
                    if (!section_offset.has_value()) {
                        return std::nullopt;
                    }

                    const auto section_size = file.read_little_endian<std::uint64_t>();
                    if (!section_size.has_value()) {
                        return std::nullopt;
                    }

                    return asset_section{
                        nro_size.value(),

                        section_offset.value(),
                        section_size.value()
                    };
                }

                std::uint32_t assets_offset;

                std::uint64_t section_offset;
                std::uint64_t section_size;

                [[nodiscard]]
                inline bool empty(this const asset_section &self) {
                    return self.section_size <= 0;
                }

                [[nodiscard]]
                inline bool seek_from_start(this const asset_section &self, util::file &file, const long offset) {
                    return file.seek_from_start(self.assets_offset + self.section_offset + offset).has_value();
                }

                [[nodiscard]]
                inline std::optional<util::unique_array<std::byte>> read_all(
                    this const asset_section &self,

                    util::file &file
                ) {
                    if (!self.seek_from_start(file, 0)) {
                        return std::nullopt;
                    }

                    auto data = util::unique_array<std::byte>(self.section_size);
                    if (!file.read_into(data).has_value()) {
                        return std::nullopt;
                    }

                    return data;
                }
            };

            std::string _path;

            constexpr const std::string &path(this const executable_app &self) {
                return self._path;
            }

            inline impl::unprocessed_metadata read_metadata(this const executable_app &self) {
                auto file = util::file::open(self._path);
                if (!file.has_value()) {
                    return {};
                }

                const auto section = asset_section::read<1>(file.value());
                if (!section.has_value()) {
                    return {};
                }

                if (section->empty()) {
                    return {};
                }

                /*
                    NOTE: We don't localize the name and author based on
                    the language because homebrew does not take advantage
                    of it anyways, and it's nice to not need to pass the
                    language to here and map the languages to the correct offset.
                */
                static constexpr long NameOffset = 0;

                if (!section->seek_from_start(file.value(), NameOffset)) {
                    return {};
                }

                static constexpr std::size_t MaxNameSize = 0x200;

                const auto name = file->read_c_str(MaxNameSize);
                if (!name.has_value()) {
                    return {};
                }

                static constexpr long AuthorOffset = MaxNameSize;

                if (!section->seek_from_start(file.value(), AuthorOffset)) {
                    return {name.value(), std::nullopt, std::nullopt};
                }

                static constexpr std::size_t MaxAuthorSize = 0x100;

                const auto author = file->read_c_str(MaxAuthorSize);
                if (!author.has_value()) {
                    return {name.value(), std::nullopt, std::nullopt};
                }

                static constexpr long VersionOffset = 0x3060;

                if (!section->seek_from_start(file.value(), VersionOffset)) {
                    return {name.value(), author.value(), std::nullopt};
                }

                static constexpr std::size_t MaxVersionSize = 0x10;

                const auto version = file->read_c_str(MaxVersionSize);
                if (!version.has_value()) {
                    return {name.value(), author.value(), std::nullopt};
                }

                return {name.value(), author.value(), version.value()};
            }

            inline std::optional<util::unique_array<std::byte>> read_icon(this const executable_app &self) {
                auto file = util::file::open(self._path);
                if (!file.has_value()) {
                    return std::nullopt;
                }

                const auto section = asset_section::read<0>(file.value());
                if (!section.has_value()) {
                    return std::nullopt;
                }

                if (section->empty()) {
                    return std::nullopt;
                }

                return section->read_all(file.value());
            }

            inline bool load(this const executable_app &self, auto &interface) {
                #if defined(__SWITCH__)

                if (!R_SUCCEEDED(envSetNextLoad(self._path.c_str(), self._path.c_str()))) {
                    return false;
                }

                #else

                LV_LOG_ERROR("Loaded executable app: %s", self._path.c_str());

                #endif

                interface.quit();

                return true;
            }
        };

        template<std::size_t NumComponents>
        constexpr std::size_t pixel_index(const std::size_t x, const std::size_t y, const std::size_t width) {
            return NumComponents * (x + width * y);
        }

        template<std::size_t DestWidth, std::size_t DestHeight, std::size_t NumComponents>
        constexpr std::unique_ptr<std::uint8_t[]> downscale_image(const std::uint8_t *src_data, const std::size_t src_width, const std::size_t src_height) {
            /*
                NOTE: This uses a fancier "unit square" version of bilinear interpolation.

                See https://en.wikipedia.org/wiki/Bilinear_interpolation#On_the_unit_square

                This code is adapted from nx-hbmenu code.
            */

            static constexpr std::size_t DownscaledDataSize = NumComponents * DestWidth * DestHeight;

            auto downscaled = std::make_unique<std::uint8_t[]>(DownscaledDataSize);

            if (src_width < DestWidth || src_height < DestHeight) {
                /*
                    We cannot upscale.

                    NOTE: In actual usage, this case will be handled
                    by the caller, who will substitute their own image.

                    But I felt it prudent to handle it
                    here as well, in the name of safety.
                */

                if constexpr (NumComponents == gui::num_argb_components) {
                    gui::fill_with_error_color(
                        std::span<std::uint8_t, DownscaledDataSize>(
                            downscaled.get(), DownscaledDataSize
                        )
                    );
                } else {
                    std::ranges::fill(std::span(downscaled.get(), DownscaledDataSize), 0xFF);
                }

                return downscaled;
            }

            const auto src = std::span(src_data, NumComponents * src_width * src_height);

            if (src_width == DestWidth && src_height == DestHeight) {
                std::ranges::copy(src, downscaled.get());

                return downscaled;
            }

            const auto x_scale = static_cast<std::float32_t>(src_width)  / DestWidth;
            const auto y_scale = static_cast<std::float32_t>(src_height) / DestHeight;

            /* NOTE: This is set up so that it will loop linearly through the buffer. */
            for (const auto [dest_y, dest_x] : std::views::cartesian_product(
                std::views::iota(0uz, DestHeight),
                std::views::iota(0uz, DestWidth)
            )) {
                const auto src_x = static_cast<std::float32_t>(dest_x) * x_scale;
                const auto src_y = static_cast<std::float32_t>(dest_y) * y_scale;

                const auto origin_x = static_cast<std::size_t>(src_x);
                const auto origin_y = static_cast<std::size_t>(src_y);

                const auto origin_index = impl::pixel_index<NumComponents>(origin_x, origin_y, src_width);

                /* This will always be true anyways, but I'm putting it here to explicate my assumption. */
                [[assume(origin_index < src.size())]];

                std::array<std::array<std::uint8_t, NumComponents>, 4> src_pixels;

                const auto store_src_pixel = [&](const std::size_t i, const std::size_t offset_x, const std::size_t offset_y) {
                    auto pixel_index = impl::pixel_index<NumComponents>(origin_x + offset_x, origin_y + offset_y, src_width);
                    if (pixel_index >= src.size()) {
                        /* Duplicate origin color as a fallback. */
                        pixel_index = origin_index;
                    }

                    std::ranges::copy(src.subspan(pixel_index, NumComponents), src_pixels[i].begin());
                };

                store_src_pixel(0, 0, 0);
                store_src_pixel(1, 1, 0);
                store_src_pixel(2, 0, 1);
                store_src_pixel(3, 1, 1);

                /* NOTE: Always in the [0, 1] range. */
                const auto origin_distance_x = src_x - static_cast<std::float32_t>(origin_x);
                const auto origin_distance_y = src_y - static_cast<std::float32_t>(origin_y);

                const auto end_distance_x = 1.0f32 - origin_distance_x;
                const auto end_distance_y = 1.0f32 - origin_distance_y;

                /* NOTE: Multiplied by 256 to later be right-shifted down by 8 bits. */
                const auto weights = std::array{
                    256.0f32 * end_distance_x    * end_distance_y,
                    256.0f32 * origin_distance_x * end_distance_y,
                    256.0f32 * end_distance_x    * origin_distance_y,
                    256.0f32 * origin_distance_x * origin_distance_y
                };

                const auto dest_index = impl::pixel_index<NumComponents>(dest_x, dest_y, DestWidth);
                for (const auto i : std::views::iota(0uz, NumComponents)) {
                    downscaled[dest_index + i] = [&]() {
                        return static_cast<std::uint8_t>(
                            static_cast<std::int16_t>(
                                weights[0] * src_pixels[0][i] +
                                weights[1] * src_pixels[1][i] +
                                weights[2] * src_pixels[2][i] +
                                weights[3] * src_pixels[3][i]
                            ) >> 8uz
                        );
                    }();
                }
            }

            return downscaled;
        }

    }

    /* Forward declare. */
    struct interface;

    /*
        NOTE: This "basic" functionality is separated out for
        remote loading, where we don't need to load all the
        fancy stuff about an app and just need to load it.
    */
    struct basic_app {
        struct metadata {
            lang::translated name;
            lang::translated author;
            lang::translated version;
        };

        using underlying = std::variant<
            impl::theme_app,
            impl::executable_app
        >;

        underlying _underlying;

        static constexpr std::optional<basic_app> from_direct_path(std::string path) {
            if (path.ends_with(impl::executable_app::extension)) {
                return basic_app{impl::executable_app{std::move(path)}};
            }

            if (path.ends_with(impl::theme_app::extension)) {
                return basic_app{impl::theme_app{std::move(path)}};
            }

            return std::nullopt;
        }

        static constexpr std::optional<basic_app> from_directory_entry(const util::directory::entry entry) {
            /* NOTE: We assume the parent directory is the apps directory. */

            const std::string_view entry_name = entry.name();

            auto path = std::string(config::paths::apps_directory) + entry_name;

            if (entry.is_directory()) {
                /* See if we have a subdirectory app. */

                path += config::path_separator;
                path += entry_name;
                path += impl::executable_app::extension;

                if (!util::file::exists(path)) {
                    return std::nullopt;
                }

                return from_direct_path(std::move(path));
            }

            return from_direct_path(std::move(path));
        }

        constexpr const std::string &path(this const basic_app &self) {
            return std::visit(
                [](const auto &underlying) -> auto & {
                    return underlying.path();
                },

                self._underlying
            );
        }

        constexpr metadata read_metadata(this const basic_app &self, const lang::language language) {
            auto unprocessed = std::visit(
                [](const auto &underlying) {
                    return underlying.read_metadata();
                },

                self._underlying
            );

            unprocessed.replace_newlines();

            return {
                .name = [&]() {
                    if (!unprocessed.name.has_value()) {
                        return lang::translated(
                            lang::translations::unknown_app_name.translate(language)
                        );
                    }

                    return lang::translated(std::move(unprocessed.name).value());
                }(),

                .author = [&]() {
                    if (!unprocessed.author.has_value()) {
                        return lang::translated(
                            lang::translations::unknown_app_author.translate(language)
                        );
                    }

                    return lang::translated(std::move(unprocessed.author).value());
                }(),

                .version = [&]() {
                    if (!unprocessed.version.has_value()) {
                        return lang::translated(
                            lang::translations::unknown_app_version.translate(language)
                        );
                    }

                    return lang::translated(std::move(unprocessed.version).value());
                }(),
            };
        }

        constexpr std::optional<util::unique_array<std::byte>> read_icon(this const basic_app &self) {
            return std::visit(
                [](const auto &underlying) {
                    return underlying.read_icon();
                },

                self._underlying
            );
        }

        constexpr std::string star_path(this const basic_app &self) {
            const auto &path = self.path();

            const auto name_start = path.begin() + [&]() {
                const auto pos = path.find_last_of(config::path_separator);
                if (pos == std::string::npos) {
                    return 0uz;
                }

                return pos + 1;
            }();

            auto star_path = std::string(path.begin(), name_start);

            star_path += '.';

            star_path.append(name_start, path.end());

            star_path += ".star";

            return star_path;
        }

        inline bool check_starred(this const basic_app &self) {
            return util::file::exists(self.star_path());
        }

        [[nodiscard]]
        inline bool remove(this basic_app &self) {
            /* TODO: Subdirectory apps. */

            /* NOTE: We don't care whether removing the star file fails. */
            (void) util::file::remove(self.star_path());

            return util::file::remove(self.path()).has_value();
        }

        [[nodiscard]]
        constexpr bool load(this const basic_app &self, gui::interface &interface) {
            return std::visit([&](const auto &underlying) {
                return underlying.load(interface);
            }, self._underlying);
        }
    };

    struct app : gui::basic_app {
        static constexpr auto small_icon_width  = decltype(gui::theme::unavailable_icon_small)::width;
        static constexpr auto small_icon_height = decltype(gui::theme::unavailable_icon_small)::height;

        static constexpr auto big_icon_width  = decltype(gui::theme::unavailable_icon_big)::width;
        static constexpr auto big_icon_height = decltype(gui::theme::unavailable_icon_big)::height;

        gui::basic_app::metadata meta;

        bool starred;

        util::jpeg_decompressor::decompressed _icon_data = {
            .data   = std::unique_ptr<std::uint8_t[]>(),
            .width  = 0,
            .height = 0,
        };

        std::unique_ptr<std::uint8_t[]> _small_icon_data = {};
        std::unique_ptr<std::uint8_t[]> _big_icon_data   = {};

        /* NOTE: The data of these icons could be from the theme instead of the above unique_ptr's. */
        lvgl::image_description _small_icon = lvgl::image_description::argb(
            small_icon_width, small_icon_height
        );

        lvgl::image_description _big_icon = lvgl::image_description::argb(
            big_icon_width, big_icon_height
        );

        constexpr app(const lang::language language, gui::basic_app &&app)
        :
            gui::basic_app(std::move(app)),

            meta(this->read_metadata(language)),

            starred(this->check_starred())
        {}

        [[nodiscard]]
        inline bool toggle_starred(this app &self) {
            const auto succeeded = [&]() {
                if (self.starred) {
                    return util::file::remove(self.star_path()).has_value();
                } else {
                    return util::file::create(self.star_path()).has_value();
                }
            }();

            if (!succeeded) {
                return false;
            }

            self.starred = !self.starred;

            return true;
        }

        inline std::expected<void, std::monostate> _load_icon(this app &self) {
            if (self._icon_data.data.get() != nullptr) {
                return {};
            }

            auto decompressor = util::jpeg_decompressor::open();
            if (!decompressor.has_value()) {
                return std::unexpected(std::monostate{});
            }

            const auto jpeg_data = self.read_icon();
            if (!jpeg_data.has_value()) {
                return std::unexpected(std::monostate{});
            }

            auto decompressed = decompressor->decompress(std::span(jpeg_data.value()), TJPF_BGRA, TJFLAG_ACCURATEDCT);
            if (!decompressed.has_value()) {
                return std::unexpected(std::monostate{});
            }

            self._icon_data = std::move(decompressed).value();

            return {};
        }

        template<std::size_t Width, std::size_t Height>
        std::optional<std::span<const std::uint8_t>> _downscale_icon(
            this app &self,

            std::unique_ptr<std::uint8_t[]> &data_holder
        ) {
            static constexpr std::size_t DownscaledDataSize = 4 * Width * Height;

            if (std::cmp_equal(self._icon_data.width, Width) && std::cmp_equal(self._icon_data.height, Height)) {
                return std::span(self._icon_data.data.get(), DownscaledDataSize);
            }

            if (std::cmp_less(self._icon_data.width, Width) || std::cmp_less(self._icon_data.height, Height)) {
                return std::nullopt;
            }

            data_holder = impl::downscale_image<Width, Height, 4>(
                self._icon_data.data.get(),

                self._icon_data.width,
                self._icon_data.height
            );

            return std::span(data_holder.get(), DownscaledDataSize);
        }

        inline void _load_small_icon(this app &self, const gui::theme &theme) {
            if (self._small_icon.data().data() != nullptr) {
                return;
            }

            const auto icon_result = self._load_icon();
            if (!icon_result.has_value()) {
                self._small_icon.set_data(theme.unavailable_icon_small.description().data());

                return;
            }

            const auto data = self._downscale_icon<small_icon_width, small_icon_height>(self._small_icon_data);

            if (!data.has_value()) {
                self._small_icon.set_data(theme.unavailable_icon_small.description().data());

                return;
            }

            self._small_icon.set_data(data.value());
        }

        inline const lvgl::image_description &small_icon(this app &self, const gui::theme &theme) {
            self._load_small_icon(theme);

            return self._small_icon;
        }

        inline void _load_big_icon(this app &self, const gui::theme &theme) {
            if (self._big_icon.data().data() != nullptr) {
                return;
            }

            const auto icon_result = self._load_icon();
            if (!icon_result.has_value()) {
                self._big_icon.set_data(theme.unavailable_icon_big.description().data());

                return;
            }

            const auto data = self._downscale_icon<big_icon_width, big_icon_height>(self._big_icon_data);

            if (!data.has_value()) {
                self._big_icon.set_data(theme.unavailable_icon_big.description().data());

                return;
            }

            self._big_icon.set_data(data.value());
        }

        inline const lvgl::image_description &big_icon(this app &self, const gui::theme &theme) {
            self._load_big_icon(theme);

            return self._big_icon;
        }

        constexpr std::strong_ordering operator <=>(this const app &self, const app &rhs) {
            /*
                Compare by star, then by name, then by version, then by author, then by path.

                NOTE: We do not compare the icon.
            */

            if (self.starred != rhs.starred) {
                if (self.starred) {
                    return std::strong_ordering::less;
                }

                return std::strong_ordering::greater;
            }

            const auto name_cmp = (self.meta.name <=> rhs.meta.name);
            if (name_cmp != 0) {
                return name_cmp;
            }

            /* NOTE: Reversed comparison so that higher versions come first. */
            const auto version_cmp = (rhs.meta.version <=> self.meta.version);
            if (version_cmp != 0) {
                return version_cmp;
            }

            const auto author_cmp = (self.meta.author <=> rhs.meta.author);
            if (author_cmp != 0) {
                return author_cmp;
            }

            return (self.path() <=> rhs.path());
        }

        constexpr bool operator ==(this const app &self, const app &rhs) {
            /* If we have the same path, then everything else should be equal as well. */
            return self.path() == rhs.path();
        }

        static inline std::vector<gui::app> get_sorted_apps(const lang::language language) {
            auto directory = util::directory::open(config::paths::apps_directory);
            if (!directory.has_value()) {
                return {};
            }

            std::vector<gui::app> apps;
            for (const auto entry : directory.value()) {
                auto app = gui::basic_app::from_directory_entry(entry);
                if (!app.has_value()) {
                    continue;
                }

                apps.emplace_back(language, std::move(app).value());
            }

            std::ranges::sort(apps);

            return apps;
        }
    };

}
