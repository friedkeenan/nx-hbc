#pragma once

#include <hbc/common.hpp>

#include <hbc/util/util.hpp>

#include <hbc/lang/lang.hpp>

namespace hbc::config {

    struct settings {
        /* TODO: Load settings. */

        lang::language language = lang::system_language();
    };

}
