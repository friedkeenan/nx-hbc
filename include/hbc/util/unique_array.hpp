#pragma once

#include <hbc/common.hpp>

namespace hbc::util {

    template<typename T>
    struct unique_array {
        std::unique_ptr<T[]> _data;
        std::size_t          _size = 0;

        constexpr unique_array() = default;

        constexpr explicit unique_array(const std::size_t size)
        :
            _data(std::make_unique<T[]>(size)),
            _size(size)
        {}

        constexpr T *data(this unique_array &self) {
            return self._data.get();
        }

        constexpr const T *data(this const unique_array &self) {
            return self._data.get();
        }

        constexpr std::size_t size(this const unique_array &self) {
            return self._size;
        }

        [[nodiscard]]
        constexpr bool empty(this const unique_array &self) {
            return self.size() <= 0;
        }

        constexpr T *begin(this unique_array &self) {
            return self.data();
        }

        constexpr const T *begin(this const unique_array &self) {
            return self.data();
        }

        constexpr T *end(this unique_array &self) {
            return self.begin() + self.size();
        }

        constexpr const T *end(this const unique_array &self) {
            return self.begin() + self.size();
        }

        constexpr T &operator [](this unique_array &self, const std::size_t index) {
            [[assume(index < self.size())]];

            return self._data[index];
        }

        constexpr const T &operator [](this const unique_array &self, const std::size_t index) {
            [[assume(index < self.size())]];

            return self._data[index];
        }
    };

    static_assert(std::ranges::contiguous_range<unique_array<int>> && std::ranges::sized_range<unique_array<int>>);

}
