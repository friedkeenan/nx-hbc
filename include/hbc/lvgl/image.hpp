#pragma once

#include <hbc/common.hpp>

#include <hbc/lvgl/object.hpp>

namespace hbc::lvgl {

    struct image_description {
        lv_image_dsc_t _description;

        constexpr image_description(const lv_image_header_t header)
        :
            _description({
                .header     = header,
                .data_size  = 0,
                .data       = nullptr,
                .reserved   = nullptr,
                .reserved_2 = nullptr,
            })
        {}

        static constexpr image_description argb(const std::uint16_t width, const std::uint16_t height) {
            return image_description(
                lv_image_header_t{
                    .magic = LV_IMAGE_HEADER_MAGIC,
                    .cf    = LV_COLOR_FORMAT_ARGB8888,
                    .flags = 0,

                    .w = width,
                    .h = height,

                    .stride = static_cast<std::uint16_t>(4 * width),

                    .reserved_2 = 0
                }
            );
        }

        constexpr const lv_image_dsc_t *underlying(this const image_description &self) {
            /* NOTE: This could become a dnagling reference. */
            return &self._description;
        }

        constexpr void set_data(this image_description &self, const std::span<const std::uint8_t> data) {
            self._description.data      = data.data();
            self._description.data_size = static_cast<std::uint32_t>(data.size());
        }

        constexpr std::span<const std::uint8_t> data(this const image_description &self) {
            return std::span(self._description.data, self._description.data_size);
        }

        constexpr std::uint16_t width(this const image_description &self) {
            return self._description.header.w;
        }

        constexpr std::uint16_t height(this const image_description &self) {
            return self._description.header.h;
        }
    };

    struct image : lvgl::object {
        image(lvgl::parentable auto &parent) : lvgl::object(parent, lv_image_create) {}

        inline void set_description(this image &self, const lvgl::image_description &description) {
            lv_image_set_src(self._object, description.underlying());

            /*
                NOTE: By default, we don't want to
                have our size expanded by our children.

                If we didn't set this, then 'lv_image' would
                by default use 'LV_SIZE_CONTENT' for its size.
            */
            self.set_size({
                static_cast<std::int32_t>(description.width()),
                static_cast<std::int32_t>(description.height()),
            });
        }
    };

}
