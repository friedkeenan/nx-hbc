#pragma once

#include <hbc/common.hpp>

namespace hbc::util {

    struct unsafe_length_t {};

    constexpr inline auto unsafe_length = util::unsafe_length_t{};

    namespace impl {

        /* Undefined function for erroring in constant evaluation. */
        void no_null_terminator_found();

    }

    template<std::size_t Size>
    struct fixed_string {
        std::array<char, Size + 1> _data;

        consteval explicit(false) fixed_string(const char (&data)[Size + 1]) {
            if (data[Size] != '\0') {
                impl::no_null_terminator_found();
            }

            /* NOTE: Will copy the null terminator. */
            std::ranges::copy(data, this->_data.begin());
        }

        consteval explicit fixed_string(const std::array<char, Size + 1> &data) : _data(data) {
            if (data.back() != '\0') {
                impl::no_null_terminator_found();
            }
        }

        constexpr const char &front(this const fixed_string &self) {
            return self._data[0];
        }

        constexpr const char &back(this const fixed_string &self) {
            return self._data[Size - 1];
        }

        consteval explicit(false) operator std::string_view(this const fixed_string &self) {
            return std::string_view(self._data.data(), Size);
        }

        constexpr const char *c_str(this const fixed_string &self) {
            return self._data.data();
        }

        template<std::size_t OtherSize>
        consteval fixed_string<Size + OtherSize> operator +(this const fixed_string &self, const fixed_string<OtherSize> &other) {
            std::array<char, Size + OtherSize + 1> data;

            std::ranges::copy_n(self._data.begin(), Size, data.begin());

            /* NOTE: Will copy the null terminator. */
            std::ranges::copy(other._data, data.begin() + Size);

            return fixed_string<Size + OtherSize>(data);
        }

        template<std::size_t OtherSize>
        consteval fixed_string<Size + OtherSize - 1> operator +(this const fixed_string &self, const char (&data)[OtherSize]) {
            return self + fixed_string<OtherSize - 1>(data);
        }
    };

    template<std::size_t N>
    fixed_string(const char (&)[N]) -> fixed_string<N - 1>;

    /* A null-terminated view of a string. */
    struct zstring : std::ranges::view_interface<zstring> {
        struct sentinel {
            constexpr bool operator ==(this sentinel, const char *it) {
                return *it == '\0';
            }
        };

        const char *_data;

        template<std::size_t N>
        consteval explicit(false) zstring(const char (&str)[N]) : _data(str + 0) {
            if (!std::ranges::contains(str, '\0')) {
                impl::no_null_terminator_found();
            }
        }

        constexpr explicit(false) zstring(const std::string &str) : _data(str.c_str()) {}

        template<std::size_t N>
        constexpr explicit(false) zstring(const util::fixed_string<N> &str) : _data(str.c_str()) {}

        constexpr zstring(util::unsafe_length_t, const char *str) : _data(str) {}

        zstring(util::unsafe_length_t, std::nullptr_t) = delete;

        constexpr const char *c_str(this const zstring self) {
            return self._data;
        }

        constexpr const char *data(this const zstring self) {
            return self._data;
        }

        /* NOTE: Not constant time. */
        constexpr std::size_t size(this const zstring self) {
            return std::char_traits<char>::length(self._data);
        }

        constexpr std::size_t length(this const zstring self) {
            return self.size();
        }

        constexpr const char *begin(this const zstring self) {
            return self._data;
        }

        constexpr sentinel end(this const zstring) {
            return sentinel{};
        }

        constexpr explicit(false) operator std::string_view(this const zstring self) {
            return std::string_view(self._data, self.size());
        }

        friend constexpr const char *format_as(const zstring self) {
            return self.c_str();
        }
    };

    static_assert(std::ranges::contiguous_range<util::zstring>);

}

namespace std::ranges {

    template<>
    constexpr inline bool disable_sized_range<hbc::util::zstring> = true;

    template<>
    constexpr inline bool enable_borrowed_range<hbc::util::zstring> = true;
}
