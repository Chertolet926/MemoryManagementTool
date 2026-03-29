#pragma once
/**
 * @file proc_utils.hpp
 * @brief Utilities for /proc filesystem operations.
 *
 * Provides functions for reading process information from /proc/[pid]/status,
 * parsing PIDs, and determining process types (user-space vs kernel threads).
 */

#include <filesystem>
#include <expected>
#include <charconv>
#include <limits>

// Integrating your existing parser infrastructure
#include "dex/kv_parser.hpp"
#include "utils/system_info.hpp"
#include "common/errors.hpp"
#include "common/aliases.hpp"
#include "common/constants.hpp"

namespace utils::proc {

    /**
     * @enum proc_errc
     * @brief Error codes for /proc filesystem operations.
     */
    enum class proc_errc {
        success = 0,           ///< Success (no error)
        file_not_found,        ///< /proc/[pid]/status does not exist
        permission_denied,     ///< Cannot read the status file
        parse_error,           ///< Failed to parse the UID value
        uid_not_found,         ///< "Uid" key missing in status file
        invalid_uid_format,    ///< UID value has invalid format
        uid_out_of_range       ///< UID value is outside valid range
    };

    /**
     * @struct proc_error_category
     * @brief Error category for proc operations.
     */
    struct proc_error_category : boost::system::error_category {
        const char* name() const noexcept override { return "dex_proc"; }

        std::string message(int ev) const override {
            switch (static_cast<proc_errc>(ev)) {
                case proc_errc::file_not_found:       return "Process status file not found";
                case proc_errc::permission_denied:    return "Permission denied reading status";
                case proc_errc::parse_error:          return "Failed to parse UID value";
                case proc_errc::uid_not_found:        return "Uid field not found in status";
                case proc_errc::invalid_uid_format:   return "UID has invalid format";
                case proc_errc::uid_out_of_range:     return "UID value out of valid range";
                default:                              return "Unknown proc error";
            }
        }
    };

    /**
     * @brief Returns a reference to the static proc error category instance.
     * @return Reference to the singleton proc_error_category.
     */
    inline const boost::system::error_category& get_proc_error_category() {
        static proc_error_category instance;
        return instance;
    }

    /**
     * @brief Factory function to create error_code from proc_errc.
     * @param e The specific proc error code.
     * @return A complete error_code object containing the value and category.
     */
    inline boost::system::error_code make_error_code(proc_errc e) {
        return {static_cast<int>(e), get_proc_error_category()};
    }

    /**
     * @brief Validates that a UID is within the acceptable range.
     * @param uid The UID to validate.
     * @return true if UID is valid, false otherwise.
     *
     * @note uid_t is unsigned by definition, so always >= 0.
     *       This function checks that the UID fits within the system's uid_t range.
     */
    [[nodiscard]] inline bool is_valid_uid(uid_t uid) noexcept {
        // uid_t is unsigned, so we only check upper bound
        return uid <= std::numeric_limits<uid_t>::max();
    }

    /**
     * @brief Extracts UID from /proc/[pid]/status using the existing KVParser.
     * @param p Path to the process directory (e.g., /proc/1234).
     * @return Real UID (ruid) or detailed error on failure.
     *
     * @note This function does NOT check file existence before parsing to avoid
     *       TOCTOU (Time-of-Check-Time-of-Use) vulnerabilities. The parser itself
     *       will handle file not found errors.
     */
    inline std::expected<uid_t, process_error> get_uid(const fs::path& p) {
        // Use safe path construction
        const fs::path status_path = p / "status";

        // Use the existing KVParser to parse the 'status' file into a KVRegistry
        // No pre-check for file existence - this avoids TOCTOU vulnerabilities
        auto result = dex::KVParser::from_file(status_path.string());

        if (!result) {
            // Parser error - could be file not found, permission denied, or parse failure
            // Check the error code to provide more specific error
            if (result.error().ec.category() == boost::system::system_category()) {
                // System error (e.g., ENOENT, EACCES)
                if (result.error().ec.value() == ENOENT) {
                    return std::unexpected(process_error{
                        .ec = make_error_code(proc_errc::file_not_found),
                        .component = "GET_UID",
                        .target_pid = 0
                    });
                }
                if (result.error().ec.value() == EACCES) {
                    return std::unexpected(process_error{
                        .ec = make_error_code(proc_errc::permission_denied),
                        .component = "GET_UID",
                        .target_pid = 0
                    });
                }
            }
            
            // Generic parser error
            return std::unexpected(process_error{
                .ec = result.error().ec,
                .component = "GET_UID_PARSE",
                .target_pid = 0
            });
        }

        // Locate the "Uid" key in the registry
        auto it = result->find("Uid");
        if (it == result->end() || it->second.empty()) {
            return std::unexpected(process_error{
                .ec = make_error_code(proc_errc::uid_not_found),
                .component = "GET_UID",
                .target_pid = 0
            });
        }

        /**
         * According to proc(5), the 'Uid' line contains 4 values:
         * Real, Effective, Saved, and File System UIDs.
         * We extract the first one (Real UID).
         */
        const auto& val = it->second[0];

        // If it's stored as a long/int in your Value variant
        if (auto* id = std::get_if<long>(&val)) {
            // Validate range before conversion
            if (*id < 0 || static_cast<unsigned long>(*id) > std::numeric_limits<uid_t>::max()) {
                return std::unexpected(process_error{
                    .ec = make_error_code(proc_errc::uid_out_of_range),
                    .component = "GET_UID_VALIDATE",
                    .target_pid = 0
                });
            }
            return static_cast<uid_t>(*id);
        }

        // Fallback: if it's stored as a string, parse it
        if (auto* s = std::get_if<std::string>(&val)) {
            unsigned long ruid;
            auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), ruid);
            if (ec == std::errc{} && ptr == s->data() + s->size()) {
                // Validate range
                if (ruid <= std::numeric_limits<uid_t>::max()) {
                    return static_cast<uid_t>(ruid);
                }
                return std::unexpected(process_error{
                    .ec = make_error_code(proc_errc::uid_out_of_range),
                    .component = "GET_UID_VALIDATE",
                    .target_pid = 0
                });
            }
        }
        
        return std::unexpected(process_error{
            .ec = make_error_code(proc_errc::invalid_uid_format),
            .component = "GET_UID_PARSE",
            .target_pid = 0
        });
    }

    /**
     * @brief Fast PID parsing using std::from_chars (no-copy, no-throw).
     * @param s String to parse as PID.
     * @return Parsed PID or std::nullopt if parsing fails.
     */
    [[nodiscard]] inline std::optional<int> to_pid(const std::string& s) {
        int pid;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), pid);
        if (ec != std::errc{} || ptr != s.data() + s.size()) {
            return std::nullopt;
        }
        // Validate PID range
        if (pid < mmt::MIN_VALID_PID || pid > mmt::MAX_VALID_PID) {
            return std::nullopt;
        }
        return pid;
    }

    /**
     * @brief Determines if a process is user-land by checking the 'exe' symlink.
     * @param p Path to the process directory.
     * @return true if the process is a user-space process, false for kernel threads.
     *
     * @note Kernel threads do not have an 'exe' symlink in /proc/[pid]/.
     */
    [[nodiscard]] inline bool is_user_land(const fs::path& p) {
        std::error_code ec;
        return fs::exists(p / "exe", ec);
    }
} // namespace utils::proc

/**
 * @brief Register proc_errc with Boost.System for automatic error code conversion.
 */
namespace boost::system {
    template <> struct is_error_code_enum<utils::proc::proc_errc> : std::true_type {};
}
