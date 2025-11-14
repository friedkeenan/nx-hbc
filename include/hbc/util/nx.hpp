#pragma once

#if defined(__SWITCH__)

#include <hbc/common.hpp>

namespace hbc::util {

    template<auto Init, auto Close>
    requires (std::invocable<decltype(Close)>)
    struct service_guard {
        HBC_NON_COPYABLE(service_guard);
        HBC_NON_MOVEABLE(service_guard);

        using expected = std::expected<service_guard, Result>;

        /* A tag required for construction that the user shouldn't access. */
        struct _private_init {};

        /*
            NOTE: We trade trivial constructibility in
            exchange for the user not being able to construct
            a 'service_guard' without accessing private entities.
        */
        constexpr explicit service_guard(_private_init) {}

        template<typename... Args>
        requires (
            std::invocable<decltype(Init), Args...> &&
            std::same_as<std::invoke_result_t<decltype(Init), Args...>, Result>
        )
        [[nodiscard]]
        static constexpr expected init(Args &&... args) {
            const auto result = std::invoke(Init, std::forward<Args>(args)...);
            if (R_FAILED(result)) {
                return std::unexpected(result);
            }

            return expected(std::in_place, _private_init{});
        }

        constexpr ~service_guard() {
            std::invoke(Close);
        }

    };

    using romfs_guard = util::service_guard<romfsInit, romfsExit>;

}

#endif
