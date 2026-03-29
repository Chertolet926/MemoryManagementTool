#pragma once
/**
 * @file process_scanner.hpp
 * @brief Process scanning utilities for /proc filesystem.
 *
 * Provides declarative process scanning with filtering capabilities
 * and full error reporting for skipped processes.
 */

#include <vector>
#include <ranges>
#include <expected>
#include "proc_utils.hpp"
#include "system_info.hpp"
#include "common/aliases.hpp"
#include "common/logger.hpp"
#include "common/constants.hpp"

namespace utils {

    /**
     * @enum ScanMode
     * @brief Filtering strategies for process scanning.
     */
    enum class ScanMode {
        OnlyMe,         ///< Show only processes owned by the current user
        AllHumanUsers,  ///< Show processes owned by users with UID >= 1000
        RootOnly,       ///< Show only processes owned by root (UID 0)
        AllUserSpace    ///< Show all user-space processes (excluding kernel threads)
    };
    
    /**
     * @struct ScanResult
     * @brief Result of a process scan operation.
     *
     * Contains both successful matches and information about skipped processes.
     */
    struct ScanResult {
        std::vector<fs::path> matched_paths;    ///< Paths of processes matching the filter
        std::vector<fs::path> skipped_paths;    ///< Paths that were skipped (with errors)

        /**
         * @brief Returns the count of matched processes.
         * @return Number of matched processes.
         */
        [[nodiscard]] size_t count() const noexcept { return matched_paths.size(); }

        /**
         * @brief Returns the count of skipped processes.
         * @return Number of skipped processes.
         */
        [[nodiscard]] size_t skipped_count() const noexcept { return skipped_paths.size(); }
    };

    /**
     * @class ProcessScanner
     * @brief Scans /proc filesystem for processes matching specified criteria.
     *
     * Provides declarative process scanning with full error reporting.
     */
    class ProcessScanner {
    public:
        /**
         * @struct Target
         * @brief Represents a scanned process target.
         */
        struct Target { int pid; fs::path path; };

        /**
         * @brief Scans /proc using a declarative approach with full error reporting.
         * @param mode Filtering strategy to apply.
         * @return ScanResult containing matched and skipped processes.
         */
        static std::expected<ScanResult, process_error> scan(ScanMode mode = ScanMode::OnlyMe) {
            MMT_LOG_DEBUG("Starting process scan with mode=", static_cast<int>(mode));
            
            ScanResult result;
            std::error_code ec;

            auto it = fs::directory_iterator("/proc", ec);
            if (ec) {
                MMT_LOG_ERROR("Failed to iterate /proc: ", ec.message());
                return std::unexpected(process_error{
                    .ec = boost::system::error_code(ec.value(), boost::system::system_category()),
                    .component = "PROC_SCAN",
                    .target_pid = 0
                });
            }

            // Get the real user UID safely (handles sudo detection)
            auto uid_result = sys::get_real_user_id_safe();
            if (!uid_result) {
                MMT_LOG_DEBUG("get_real_user_id_safe() returned error, falling back to getuid()");
                uid_result = getuid();
            }
            uid_t my_uid = uid_result.value();
            MMT_LOG_TRACE("Scanning for UID=", my_uid, ", mode=", static_cast<int>(mode));

            // Process loop: Filter directories -> Extract PID -> Apply Mode Filter
            size_t scanned = 0;
            for (const auto& entry : it) {
                const auto& path = entry.path();
                if (!entry.is_directory()) continue;

                auto pid_opt = proc::to_pid(path.filename().string());
                if (!pid_opt) continue;  // Not a PID directory

                // Check if user-land process
                if (!proc::is_user_land(path)) continue;

                ++scanned;

                // Get UID with full error reporting
                auto p_uid_result = proc::get_uid(path);
                if (!p_uid_result) {
                    // Record skipped process for diagnostics
                    MMT_LOG_TRACE("Skipping PID ", *pid_opt, ": ", p_uid_result.error().ec.message());
                    result.skipped_paths.push_back(path);
                    continue;
                }

                uid_t p_uid = p_uid_result.value();

                // Apply mode filter
                bool matches = false;
                switch (mode) {
                    case ScanMode::OnlyMe:        matches = (p_uid == my_uid); break;
                    case ScanMode::AllHumanUsers: matches = (p_uid >= mmt::MIN_HUMAN_USER_UID); break;
                    case ScanMode::RootOnly:      matches = (p_uid == mmt::ROOT_UID); break;
                    case ScanMode::AllUserSpace:  matches = true; break;
                }

                if (matches) {
                    MMT_LOG_TRACE("Matched PID ", *pid_opt, " (UID=", p_uid, ", mode=", static_cast<int>(mode));
                    result.matched_paths.push_back(path);
                }
            }

            MMT_LOG_INFO("Scan complete: found ", result.count(), " processes (scanned ", scanned, ", skipped ", result.skipped_count(), ")");
            return result;
        }

        /**
         * @brief Scans /proc and returns matched processes (legacy API).
         * @param mode Filtering strategy to apply.
         * @return Vector of Target structures containing PID and path.
         * @deprecated Use scan() for error diagnostics.
         */
        [[deprecated("Use scan() for full error reporting")]]
        static std::vector<Target> get_pids(ScanMode mode = ScanMode::OnlyMe) {
            std::vector<Target> results;
            
            auto scan_result = scan(mode);
            if (!scan_result) {
                return results;  // Return empty on error
            }
            
            for (const auto& path : scan_result->matched_paths) {
                if (auto pid = proc::to_pid(path.filename().string())) {
                    results.push_back({ *pid, path });
                }
            }
            
            return results;
        }
    };
} // namespace utils
