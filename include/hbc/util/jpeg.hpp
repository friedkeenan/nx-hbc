#pragma once

#include <hbc/common.hpp>

namespace hbc::util {

    /* NOTE: The portlib for Switch only has libjpeg-turbo at version 2.1.0. */
    struct jpeg_decompressor {
        HBC_NON_COPYABLE(jpeg_decompressor);

        struct decompressed {
            std::unique_ptr<std::uint8_t[]> data;
            int width;
            int height;
        };

        tjhandle _handle;

        static inline std::expected<jpeg_decompressor, std::monostate> open() {
            auto handle = tjInitDecompress();
            if (handle == nullptr) {
                return std::unexpected(std::monostate{});
            }

            return jpeg_decompressor(handle);
        }

        constexpr jpeg_decompressor(tjhandle handle) : _handle(handle) {}

        constexpr void _destroy(this jpeg_decompressor &self) {
            if (self._handle == nullptr) {
                return;
            }

            tjDestroy(self._handle);
        }

        constexpr jpeg_decompressor(jpeg_decompressor &&other) noexcept : _handle(std::exchange(other._handle, nullptr)) {}

        constexpr jpeg_decompressor &operator =(this jpeg_decompressor &self, jpeg_decompressor &&other) noexcept {
            self._destroy();

            self._handle = std::exchange(other._handle, nullptr);

            return self;
        }

        constexpr ~jpeg_decompressor() {
            this->_destroy();
        }

        inline std::expected<decompressed, int> decompress(
            this jpeg_decompressor &self,

            const std::span<const std::byte> jpeg_data,
            const int pixel_format,
            const int flags
        ) {
            int width;
            int height;

            /* Dummy object because we don't care about all the header information. */
            int dummy;

            const auto header_result = tjDecompressHeader3(
                self._handle,

                reinterpret_cast<const unsigned char *>(jpeg_data.data()),
                jpeg_data.size(),

                &width,
                &height,

                &dummy,
                &dummy
            );

            if (header_result != 0) {
                return std::unexpected(header_result);
            }

            if (width < 0) {
                return std::unexpected(width);
            }

            if (height < 0) {
                return std::unexpected(height);
            }

            const auto pixel_size = tjPixelSize[pixel_format];
            auto raw_data = std::make_unique<std::uint8_t[]>(pixel_size * width * height);

            const auto decompress_result = tjDecompress2(
                self._handle,

                reinterpret_cast<const unsigned char *>(jpeg_data.data()),
                jpeg_data.size(),

                raw_data.get(),
                0,
                0,
                0,

                pixel_format,
                flags
            );

            if (decompress_result != 0) {
                return std::unexpected(decompress_result);
            }

            return decompressed{
                .data   = std::move(raw_data),
                .width  = width,
                .height = height
            };
        }
    };

}
