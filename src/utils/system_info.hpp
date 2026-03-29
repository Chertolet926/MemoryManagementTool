#pragma once

#include <unistd.h>
#include <cstdlib>
#include <string>

namespace utils::sys {

    /** @brief Returns the UID of the actual human user, even if running under sudo. */
    inline uid_t get_real_user_id() {
        const char* sudo_uid = std::getenv("SUDO_UID");
        return sudo_uid ? static_cast<uid_t>(std::stoul(sudo_uid)) : getuid();
    }

}