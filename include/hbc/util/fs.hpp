#pragma once

#include <minizip/unzip.h>

#include <hbc/common.hpp>

#include <hbc/config/paths.hpp>

#include <hbc/util/string.hpp>
#include <hbc/util/unique_array.hpp>

namespace hbc::util {

    struct file {
        HBC_NON_COPYABLE(file);

        static inline bool exists(const util::zstring path) {
            struct stat info;

            if (stat(path.c_str(), &info) != 0) {
                return false;
            }

            /* Check that it is indeed a file specifically. */
            return (info.st_mode & S_IFREG) != 0;
        }

        [[nodiscard]]
        static inline std::expected<void, std::monostate> copy_to(const util::zstring src_path, const util::zstring dest_path) {
            static constexpr std::size_t BufferSize = 1024 * 1024;

            auto src = file::open(src_path);
            if (!src.has_value()) {
                return std::unexpected(std::monostate{});
            }

            auto dest = file::open(dest_path, "wb");
            if (!dest.has_value()) {
                return std::unexpected(std::monostate{});
            }

            const auto buffer = std::make_unique<std::byte[]>(BufferSize);

            while (true) {
                const auto read_result = src->read_into(std::span(buffer.get(), BufferSize));

                if (!read_result.has_value()) {
                    if (src->reached_eof()) {
                        const auto read_size = read_result.error();

                        if (!dest->write(std::span(buffer.get(), read_size)).has_value()) {
                            return std::unexpected(std::monostate{});
                        }

                        return {};
                    }

                    return std::unexpected(std::monostate{});
                }

                if (!dest->write(std::span(buffer.get(), BufferSize)).has_value()) {
                    return std::unexpected(std::monostate{});
                }
            }
        }

        [[nodiscard]]
        static inline std::expected<void, std::monostate> create(const util::zstring path) {
            if (!file::open(path, "wb").has_value()) {
                return std::unexpected(std::monostate{});
            }

            return {};
        }

        [[nodiscard]]
        static inline std::expected<void, int> remove(const util::zstring path) {
            const auto result = std::remove(path.c_str());
            if (result != 0) {
                return std::unexpected(result);
            }

            return {};
        }

        std::FILE *_file;

        constexpr explicit file(std::FILE *fp) : _file(fp) {}

        /* NOTE: By default open the file with the 'rb' mode. */
        static inline std::expected<file, std::monostate> open(const util::zstring path, const util::zstring mode = "rb") {
            const auto fp = std::fopen(path.c_str(), mode.c_str());
            if (fp == nullptr) {
                return std::unexpected(std::monostate{});
            }

            return file(fp);
        }

        constexpr void _close(this file &self) {
            if (self._file == nullptr) {
                return;
            }

            std::fclose(self._file);
        }

        constexpr file(file &&other) noexcept : _file(std::exchange(other._file, nullptr)) {}

        constexpr file &operator =(this file &self, file &&other) noexcept {
            self._close();

            self._file = std::exchange(other._file, nullptr);

            return self;
        }

        constexpr ~file() {
            this->_close();
        }

        [[nodiscard]]
        inline std::expected<void, std::size_t> read_into(this file &self, const std::span<std::byte> data) {
            const auto read_size = std::fread(data.data(), 1, data.size(), self._file);

            if (read_size != data.size()) {
                return std::unexpected(read_size);
            }

            return {};
        }

        template<typename T>
        requires (std::is_trivially_copyable_v<T>)
        [[nodiscard]]
        inline std::expected<T, std::size_t> bit_read(this file &self) {
            std::array<std::byte, sizeof(T)> data;

            const auto result = self.read_into(data);
            if (!result.has_value()) {
                return std::unexpected(result.error());
            }

            return std::bit_cast<T>(data);
        }

        template<std::integral T>
        [[nodiscard]]
        inline std::expected<T, std::size_t> read_little_endian(this file &self) {
            std::array<std::byte, sizeof(T)> data;

            const auto result = self.read_into(data);
            if (!result.has_value()) {
                return std::unexpected(result.error());
            }

            auto number = T{};
            for (const auto i : std::views::iota(0uz, data.size())) {
                number |= (static_cast<T>(data[i]) << (CHAR_BIT * i));
            }

            return number;
        }

        [[nodiscard]]
        inline std::expected<char, std::monostate> read_char(this file &self) {
            const auto result = std::fgetc(self._file);
            if (result == EOF) {
                return std::unexpected(std::monostate{});
            }

            return static_cast<char>(result);
        }

        [[nodiscard]]
        inline std::expected<std::string, std::monostate> read_c_str(this file &self, std::size_t max_size) {
            std::string result;

            while (max_size > 0) {
                const auto c = self.read_char();
                if (!c.has_value()) {
                    return std::unexpected(std::monostate{});
                }

                if (c.value() == '\0') {
                    break;
                }

                result += c.value();

                --max_size;
            }

            return result;
        }

        [[nodiscard]]
        inline std::expected<void, std::size_t> write(this file &self, const std::span<const std::byte> data) {
            const auto write_size = std::fwrite(data.data(), 1, data.size(), self._file);

            if (write_size != data.size()) {
                return std::unexpected(write_size);
            }

            return {};
        }

        inline bool reached_eof(this const file &self) {
            return std::feof(self._file) != 0;
        }

        [[nodiscard]]
        inline std::expected<void, int> seek(this file &self, const long offset, const int origin = SEEK_CUR) {
            const auto result = std::fseek(self._file, offset, origin);
            if (result != 0) {
                return std::unexpected(result);
            }

            return {};
        }

        [[nodiscard]]
        inline std::expected<void, int> seek_from_start(this file &self, const long offset) {
            return self.seek(offset, SEEK_SET);
        }

        /* TODO: Check if we need this. */
        [[nodiscard]]
        inline std::expected<long, std::monostate> size(this const file &self) {
            /*
                NOTE: We don't use our 'seek' method so that
                this method can be called on a const file.
            */

            const auto original_pos = std::ftell(self._file);
            if (original_pos < 0) {
                return std::unexpected(std::monostate{});
            }

            if (std::fseek(self._file, 0, SEEK_END) != 0) {
                return std::unexpected(std::monostate{});
            }

            const auto size = std::ftell(self._file);
            if (size < 0) {
                return std::unexpected(std::monostate{});
            }

            if (std::fseek(self._file, original_pos, SEEK_SET) != 0) {
                return std::unexpected(std::monostate{});
            }

            return size;
        }
    };

    struct zip_file {
        HBC_NON_COPYABLE(zip_file);

        enum class case_sensitivity : int {
            system   = 0,
            enabled  = 1,
            disabled = 2,
        };

        struct _current_file {
            HBC_NON_COPYABLE(_current_file);
            HBC_NON_MOVEABLE(_current_file);

            const zip_file &_parent;

            constexpr _current_file(const zip_file &parent) : _parent(parent) {}

            inline ~_current_file() {
                /* NOTE: We ignore the result of closing. */
                unzCloseCurrentFile(this->_parent._file);
            }

            [[nodiscard]]
            inline std::expected<unsigned long, int> size(this const _current_file &self) {
                unz_file_info info;

                const auto result = unzGetCurrentFileInfo(self._parent._file, &info, nullptr, 0, nullptr, 0, nullptr, 0);
                if (result != UNZ_OK) {
                    return std::unexpected(result);
                }

                return info.uncompressed_size;
            }

            [[nodiscard]]
            inline std::expected<void, int> read_into(this _current_file &self, const std::span<std::byte> data) {
                LV_ASSERT(
                    std::cmp_less_equal(data.size(), std::numeric_limits<unsigned int>::max())
                );

                const auto read_size = unzReadCurrentFile(self._parent._file, data.data(), static_cast<unsigned int>(data.size()));
                if (std::cmp_less(read_size, data.size())) {
                    return std::unexpected(read_size);
                }

                return {};
            }
        };

        unzFile _file;

        constexpr explicit zip_file(unzFile file) : _file(file) {}

        static inline std::expected<zip_file, std::monostate> open(const util::zstring path) {
            const auto file = unzOpen(path.c_str());
            if (file == nullptr) {
                return std::unexpected(std::monostate{});
            }

            return zip_file(file);
        }

        constexpr void _close(this zip_file &self) {
            if (self._file == nullptr) {
                return;
            }

            unzClose(self._file);
        }

        constexpr zip_file(zip_file &&other) noexcept : _file(std::exchange(other._file, nullptr)) {}

        constexpr zip_file &operator =(this zip_file &self, zip_file &&other) noexcept {
            self._close();

            self._file = std::exchange(other._file, nullptr);

            return self;
        }

        constexpr ~zip_file() {
            this->_close();
        }

        [[nodiscard]]
        inline std::expected<void, int> locate_file(
            this zip_file &self,

            const util::zstring    file_name,
            const case_sensitivity sensitivity = case_sensitivity::enabled
        ) {
            const auto result = unzLocateFile(self._file, file_name.c_str(), std::to_underlying(sensitivity));
            if (result != UNZ_OK) {
                return std::unexpected(result);
            }

            return {};
        }

        template<typename SuccessFn, typename ErrorFn>
        requires (
            std::invocable<SuccessFn, _current_file &> &&
            std::invocable<ErrorFn,   const int     &> &&

            std::same_as<
                std::invoke_result_t<SuccessFn, _current_file &>,
                std::invoke_result_t<ErrorFn,   const int     &>
            >
        )
        auto open_current_file(this const zip_file &self, SuccessFn &&success_fn, ErrorFn &&error_fn) {
            const auto result = unzOpenCurrentFile(self._file);
            if (result != UNZ_OK) {
                return std::invoke(std::forward<ErrorFn>(error_fn), result);
            }

            auto current_file = _current_file{self};
            return std::invoke(std::forward<SuccessFn>(success_fn), current_file);
        }

        [[nodiscard]]
        inline std::expected<std::string, int> read_current_file_text(this const zip_file &self) {
            return self.open_current_file(
                [](auto &file) -> std::expected<std::string, int> {
                    const auto size = file.size();
                    if (!size.has_value()) {
                        return std::unexpected(size.error());
                    }

                    std::string contents;
                    std::expected<void, int> read_result;
                    contents.resize_and_overwrite(size.value(), [&](char *data, const std::size_t size) {
                        read_result = file.read_into(
                            std::as_writable_bytes(
                                std::span(data, size)
                            )
                        );

                        if (!read_result.has_value()) {
                            return 0uz;
                        }

                        return size;
                    });

                    if (!read_result.has_value()) {
                        return std::unexpected(read_result.error());
                    }

                    return contents;
                },

                [](const int error) -> std::expected<std::string, int> {
                    return std::unexpected(error);
                }
            );
        }

        [[nodiscard]]
        inline std::expected<util::unique_array<std::byte>, int> read_current_file_bytes(this const zip_file &self) {
            return self.open_current_file(
                [](auto &file) -> std::expected<util::unique_array<std::byte>, int> {
                    const auto size = file.size();
                    if (!size.has_value()) {
                        return std::unexpected(size.error());
                    }

                    auto contents = util::unique_array<std::byte>(size.value());

                    const auto read_result = file.read_into(std::span(contents));
                    if (!read_result.has_value()) {
                        return std::unexpected(read_result.error());
                    }

                    return contents;
                },

                [](const int error) -> std::expected<util::unique_array<std::byte>, int> {
                    return std::unexpected(error);
                }
            );
        }
    };

    struct directory : std::ranges::view_interface<directory> {
        HBC_NON_COPYABLE(directory);

        struct entry {
            const struct dirent *_entry;

            constexpr util::zstring name(this const entry self) {
                return util::zstring(util::unsafe_length, self._entry->d_name);
            }

            constexpr bool is_directory(this const entry self) {
                /* TODO: Check if this actually gets populated on Switch. */

                return self._entry->d_type == DT_DIR;
            }

            constexpr bool is_current_directory(this const entry self) {
                static constexpr std::string_view CurrentDirectoryName = ".";

                return self.name() == CurrentDirectoryName;
            }

            constexpr bool is_parent_directory(this const entry self) {
                static constexpr std::string_view ParentDirectoryName = "..";

                return self.name() == ParentDirectoryName;
            }
        };

        struct iterator {
            HBC_NON_COPYABLE(iterator);

            using iterator_concept = std::input_iterator_tag;
            using difference_type  = std::ptrdiff_t;
            using value_type       = directory::entry;
            using reference_type   = const directory::entry &;

            DIR *_directory;
            directory::entry _entry;

            constexpr iterator(iterator &&) = default;
            constexpr iterator &operator =(iterator &&) = default;

            constexpr explicit iterator(directory &directory)
            :
                _directory(directory._directory),
                _entry(readdir(this->_directory))
            {}

            constexpr ~iterator() = default;

            constexpr reference_type operator *(this const iterator &self) {
                return self._entry;
            }

            constexpr const directory::entry *operator ->(this const iterator &self) {
                return &self._entry;
            }

            constexpr iterator &operator ++(this iterator &self) {
                while (true) {
                    self._entry._entry = readdir(self._directory);

                    if (self._entry._entry == nullptr) {
                        return self;
                    }

                    if (!self._entry.is_current_directory() && !self._entry.is_parent_directory()) {
                        return self;
                    }
                }
            }

            constexpr void operator ++(this iterator &self, int) {
                ++self;
            }

            constexpr bool operator ==(this const iterator &self, std::default_sentinel_t) {
                return self._entry._entry == nullptr;
            }
        };

        [[nodiscard]]
        static inline std::expected<void, int> ensure_exists(std::string_view path) {
            static constexpr mode_t AllPermissions = S_IRWXU | S_IRWXG | S_IRWXO;

            if (path.ends_with(config::path_separator)) {
                path.remove_suffix(1);
            }

            std::string cumulative;

            for (auto part : path | std::views::split(config::path_separator)) {
                cumulative.append_range(std::move(part));

                cumulative += config::path_separator;

                const auto result = mkdir(cumulative.c_str(), AllPermissions);
                if (result != 0 && errno != EEXIST) {
                    return std::unexpected(result);
                }
            }

            return {};
        }

        DIR *_directory;

        static inline std::expected<directory, std::monostate> open(const util::zstring path) {
            const auto dir = opendir(path.c_str());
            if (dir == nullptr) {
                return std::unexpected(std::monostate{});
            }

            return directory(dir);
        }

        constexpr explicit directory(DIR *directory) : _directory(directory) {}

        constexpr void _close(this directory &self) {
            if (self._directory == nullptr) {
                return;
            }

            closedir(self._directory);
        }

        constexpr directory(directory &&other) noexcept : _directory(std::exchange(other._directory, nullptr)) {}

        constexpr directory &operator =(this directory &self, directory &&other) noexcept {
            self._close();

            self._directory = std::exchange(other._directory, nullptr);

            return self;
        }

        constexpr ~directory() {
            this->_close();
        }

        constexpr iterator begin(this directory &self) {
            return iterator(self);
        }

        constexpr std::default_sentinel_t end(this const directory &) {
            return std::default_sentinel;
        }
    };

    static_assert(std::ranges::input_range<util::directory>);
    static_assert(std::ranges::view<util::directory>);

}
