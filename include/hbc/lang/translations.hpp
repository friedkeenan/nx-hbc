#pragma once

#include <hbc/common.hpp>

#include <hbc/lang/core.hpp>

namespace hbc::lang::translations {

    constexpr inline auto dialog_title_no_apps = lang::translation_set<>{
        .en_US = "No Applications!",
    };

    constexpr inline auto dialog_text_no_apps = lang::translation_set<>{
        .en_US = (
            "You have no applications!\n"
            "Please put your applications under the following directory:\n\n"
            HBC_APPS_DIR
        ),
    };

    constexpr inline auto unknown_app_name = lang::translation_set<>{
        .en_US = "Unknown Name",
    };

    constexpr inline auto unknown_app_author = lang::translation_set<>{
        .en_US = "Unknown Author",
    };

    constexpr inline auto unknown_app_version = lang::translation_set<>{
        .en_US = "Unknown Version",
    };

    constexpr inline auto app_version_description = lang::translation_set<const lang::translated &>{
        .en_US = "Version: {0}",
    };

    constexpr inline auto app_author_description = lang::translation_set<const lang::translated &>{
        .en_US = "Author: {0}",
    };

    constexpr inline auto dialog_button_load = lang::translation_set<>{
        .en_US = "Load",
    };

    constexpr inline auto dialog_button_star = lang::translation_set<>{
        .en_US = "Star",
    };

    constexpr inline auto dialog_button_unstar = lang::translation_set<>{
        .en_US = "Unstar",
    };

    constexpr inline auto dialog_button_delete = lang::translation_set<>{
        .en_US = "Delete",
    };

    constexpr inline auto dialog_button_back = lang::translation_set<>{
        .en_US = "Back",
    };

    constexpr inline auto dialog_title_error = lang::translation_set<>{
        .en_US = "Error",
    };

    constexpr inline auto dialog_button_ok = lang::translation_set<>{
        .en_US = "OK",
    };

    constexpr inline auto dialog_title_confirmation = lang::translation_set<>{
        .en_US = "Confirmation",
    };

    constexpr inline auto dialog_button_yes = lang::translation_set<>{
        .en_US = "Yes",
    };

    constexpr inline auto dialog_button_no = lang::translation_set<>{
        .en_US = "No",
    };

    constexpr inline auto dialog_text_app_deletion = lang::translation_set<>{
        .en_US = "Do you really want to delete this application?",
    };

    constexpr inline auto dialog_error_load = lang::translation_set<>{
        .en_US = "Loading application failed!",
    };

    constexpr inline auto dialog_error_star = lang::translation_set<>{
        .en_US = "Starring application failed!",
    };

    constexpr inline auto dialog_error_unstar = lang::translation_set<>{
        .en_US = "Unstarring application failed!",
    };

    constexpr inline auto dialog_error_delete = lang::translation_set<>{
        .en_US = "Deleting application failed!",
    };

}
