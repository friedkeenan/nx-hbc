#pragma once

#include <hbc/common.hpp>

#include <hbc/util/util.hpp>

namespace hbc::lang {

    enum class language : std::uint8_t {
        en_US,
    };

    constexpr lang::language system_language() {
        using enum lang::language;

        #if defined(__SWITCH__)

        const auto set_guard = util::service_guard<setInitialize, setExit>::init();
        if (!set_guard.has_value()) {
            return en_US;
        }

        std::uint64_t language_code;
        if (R_FAILED(setGetSystemLanguage(&language_code))) {
            return en_US;
        }

        SetLanguage language;
        if (R_FAILED(setMakeLanguage(language_code, &language))) {
            return en_US;
        }

        /* TODO: More language support. */
        switch (language) {
            default:
            case SetLanguage_ENUS:
                return en_US;
        }

        #else

        /*
            NOTE: I'm not going to bother to actually
            get the system language when on PC. Gross.
        */
        return en_US;

        #endif
    }

    /* Forward declare. */
    struct translated_view;

    struct translated {
        std::string value;

        constexpr explicit translated(std::string value) : value(std::move(value)) {}

        constexpr explicit translated(const std::same_as<lang::translated_view> auto translated) : value(translated.value) {}

        constexpr const char *c_str(this const translated &self) {
            return self.value.c_str();
        }

        constexpr auto operator <=>(const translated &) const = default;

        friend constexpr const std::string &format_as(const translated &self) {
            return self.value;
        }
    };

    struct translated_view {
        /* LVGL sadly expects null-terminated strings. */
        util::zstring value;

        constexpr explicit translated_view(const util::zstring value) : value(value) {}

        constexpr explicit(false) translated_view(const lang::translated &translated) : value(translated.value) {}

        constexpr const char *c_str(this const translated_view self) {
            return self.value.c_str();
        }

        friend constexpr util::zstring format_as(const translated_view self) {
            return self.value;
        }
    };

    template<typename... Args>
    struct translation_set {
        using string = std::conditional_t<
            (sizeof...(Args) > 0),

            fmt::format_string<Args...>,

            util::zstring
        >;

        /* TODO: Generate this stuff with reflection... hopefully. */

        string en_US;

        constexpr string _language_string(this const translation_set &self, const lang::language language) {
            switch (language) {
                using enum lang::language;

                default:
                case en_US: return self.en_US;
            }
        }

        constexpr auto translate(this const translation_set &self, const lang::language language, Args &&... args) {
            const auto string = self._language_string(language);

            if constexpr (sizeof...(Args) > 0) {
                return lang::translated(
                    fmt::format(string, std::forward<Args>(args)...)
                );
            } else {
                return lang::translated_view(string);
            }
        }
    };

}
