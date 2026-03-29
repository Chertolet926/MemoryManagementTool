#pragma once
/**
 * @file system_info.hpp
 * @brief System information utilities for secure UID detection and sudo verification.
 *
 * Provides safe functions to determine the real user identity, even when
 * running under sudo, and to verify sudo context securely.
 */

#include <unistd.h>
#include <cstdlib>
#include <string>
#include <expected>
#include <charconv>
#include <cstring>
#include <limits>
#include "common/errors.hpp"
#include "common/constants.hpp"

namespace utils::sys {

    /**
     * @brief Checks if the current process is running under sudo.
     *
     * Verifies sudo context by checking:
     * 1. SUDO_UID environment variable exists
     * 2. Parent process executable is actually /usr/bin/sudo (or similar)
     *
     * Security considerations:
     * - Uses /proc/[ppid]/exe symlink (harder to spoof than comm)
     * - Validates full path, not just substring
     * - Checks for path traversal attempts ("..")
     * - Validates no null byte injection
     *
     * @return true if running under sudo, false otherwise.
     */
    [[nodiscard]] inline bool is_running_under_sudo() noexcept {
        // Check if SUDO_UID environment variable exists
        const char* sudo_uid = std::getenv("SUDO_UID");
        if (!sudo_uid || sudo_uid[0] == '\0') {
            return false;
        }

        // Verify parent process by checking /proc/[ppid]/exe symlink
        // Store ppid immediately to avoid race condition
        const pid_t ppid = getppid();

        // Construct path using a safe method
        char exe_link_path[64];
        int written = std::snprintf(exe_link_path, sizeof(exe_link_path),
                                     "/proc/%d/exe", static_cast<int>(ppid));

        if (written < 0 || static_cast<size_t>(written) >= sizeof(exe_link_path)) {
            return false;  // Path construction failed
        }

        // Read the symlink target
        char target_path[mmt::MAX_EXE_PATH];
        ssize_t len = readlink(exe_link_path, target_path, sizeof(target_path) - 1);

        if (len < 0) {
            // Cannot read exe link - fall back to comm check (less secure)
            char comm_path[64];
            written = std::snprintf(comm_path, sizeof(comm_path),
                                    "/proc/%d/comm", static_cast<int>(ppid));
            if (written < 0 || static_cast<size_t>(written) >= sizeof(comm_path)) {
                return false;
            }

            FILE* f = std::fopen(comm_path, "r");
            if (!f) {
                return false;
            }

            char buffer[mmt::MAX_EXE_PATH] = {};
            bool is_sudo = false;

            if (std::fgets(buffer, sizeof(buffer), f)) {
                // Check for exact "sudo" or "sudo:" prefix (systemd style)
                is_sudo = (std::strncmp(buffer, "sudo", 4) == 0);
            }

            std::fclose(f);
            return is_sudo;
        }

        // Null-terminate the path
        target_path[len] = '\0';

        // SECURITY: Validate the symlink target
        std::string_view exe_path(target_path, static_cast<size_t>(len));

        // Check 1: Must be an absolute path (starts with '/')
        if (exe_path.empty() || exe_path[0] != '/') {
            return false;
        }

        // Check 2: No path traversal attempts (no ".." components)
        if (exe_path.find("..") != std::string_view::npos) {
            return false;
        }

        // Check 3: No null bytes in the path (null byte injection attack)
        if (std::strlen(target_path) != static_cast<size_t>(len)) {
            return false;
        }

        // Check 4: Path ends with "/sudo" (handles /usr/bin/sudo, /bin/sudo, etc.)
        constexpr std::string_view sudo_suffix = "/sudo";
        if (exe_path.size() < sudo_suffix.size()) {
            return false;
        }

        // Check if path ends with "/sudo"
        return exe_path.substr(exe_path.size() - sudo_suffix.size()) == sudo_suffix;
    }

    /**
     * @brief Safely parses a UID string without throwing exceptions.
     * @param uid_str The UID string to parse.
     * @return Parsed UID or error on parse failure.
     */
    inline std::expected<uid_t, process_error> parse_uid_safe(const char* uid_str) noexcept {
        if (!uid_str || uid_str[0] == '\0') {
            return std::unexpected(process_error{
                .ec = make_error_code(process_errc::access_denied),
                .component = "UID_PARSE",
                .target_pid = 0
            });
        }

        // Use std::from_chars for exception-free parsing
        uid_t result = 0;
        const char* begin = uid_str;
        const char* end = uid_str + std::strlen(uid_str);

        auto [ptr, ec] = std::from_chars(begin, end, result);

        if (ec == std::errc{} && ptr == end) {
            // Additional validation: ensure UID is within valid range
            if (result <= std::numeric_limits<uid_t>::max()) {
                return result;
            }
        }

        return std::unexpected(process_error{
            .ec = make_error_code(process_errc::access_denied),
            .component = "UID_PARSE",
            .target_pid = 0
        });
    }

    /**
     * @brief Returns the UID of the actual human user, even if running under sudo.
     *
     * This function safely determines the real user UID by:
     * 1. Checking if we're actually running under sudo (via parent process check)
     * 2. Parsing SUDO_UID without exceptions using std::from_chars
     * 3. Falling back to getuid() if sudo detection fails
     *
     * @return Real user UID, or getuid() on any error.
     */
    inline std::expected<uid_t, process_error> get_real_user_id_safe() noexcept {
        // Only trust SUDO_UID if we verify we're actually running under sudo
        if (!is_running_under_sudo()) {
            return getuid();
        }
        
        const char* sudo_uid = std::getenv("SUDO_UID");
        auto parsed = parse_uid_safe(sudo_uid);
        
        if (parsed.has_value()) {
            return parsed.value();
        }

        // Fallback to getuid() on parse failure
        return getuid();
    }

    /**
     * @brief Returns the UID of the actual human user (legacy API).
     * @return Real user UID or getuid() on error.
     * @deprecated Use get_real_user_id_safe() for proper error handling.
     *
     * This function is kept for backward compatibility but may return
     * incorrect results if SUDO_UID is malformed.
     */
    [[deprecated("Use get_real_user_id_safe() for proper error handling")]]
    inline uid_t get_real_user_id() noexcept {
        auto result = get_real_user_id_safe();
        return result.value_or(getuid());
    }

} // namespace utils::sys
