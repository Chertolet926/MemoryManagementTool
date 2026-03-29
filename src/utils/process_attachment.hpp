#pragma once
/**
 * @file process_attachment.hpp
 * @brief RAII guard for ptrace-based process attachment.
 *
 * Provides safe process attachment with automatic cleanup to prevent
 * processes from being left in stopped state.
 */

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <expected>
#include "common/errors.hpp"
#include "common/aliases.hpp"
#include "common/logger.hpp"
#include "common/constants.hpp"
#include <thread>
#include <chrono>

namespace utils {

    /**
     * @brief Validates that a PID is within the acceptable range.
     * @param pid Process ID to validate.
     * @return true if PID is valid, false otherwise.
     */
    inline bool is_valid_pid(pid_t pid) noexcept {
        return pid >= mmt::MIN_VALID_PID && pid <= mmt::MAX_VALID_PID;
    }

    /**
     * @class ProcessAttachment
     * @brief RAII guard for ptrace-based process suspension.
     *
     * Ensures that the target process is always resumed (detached) when the guard
     * goes out of scope, preventing processes from being stuck in "T" (stopped) state.
     *
     * @note This class is move-only to prevent double-detaching.
     */
    class ProcessAttachment {
    private:
        int target_pid;
        bool is_active = false;

        // Private constructor: enforce creation through the 'attach' static factory
        explicit ProcessAttachment(int pid) : target_pid(pid), is_active(true) {}

    public:
        /**
         * @brief Attaches to a process and synchronizes its stop state.
         * @param pid Process ID to attach to.
         * @return Expected containing the Guard or a detailed error.
         *
         * @note This function performs anti-PID-recycling validation using PTRACE_GETPID.
         */
        [[nodiscard]] static std::expected<ProcessAttachment, process_error> attach(int pid) {
            MMT_LOG_DEBUG("Attempting to attach to PID ", pid);
            
            // Validate PID range before any operations
            if (!is_valid_pid(pid)) {
                MMT_LOG_WARN("PID ", pid, " is out of valid range (1-", MAX_VALID_PID, ")");
                return std::unexpected(process_error{
                    .ec = make_error_code(process_errc::process_not_found),
                    .component = "PID_VALIDATION",
                    .target_pid = pid
                });
            }

            // 1. ATTACH: Send SIGSTOP to the target via ptrace.
            // This is the first point of failure (e.g., EPERM if not root, ESRCH if process is gone).
            MMT_LOG_TRACE("Calling PTRACE_ATTACH for PID ", pid);
            if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
                MMT_LOG_ERROR("PTRACE_ATTACH failed for PID ", pid, ": errno=", errno);
                return std::unexpected(process_error{
                    .ec = boost::system::error_code(errno, boost::system::system_category()),
                    .component = "PTRACE_ATTACH",
                    .target_pid = pid
                });
            }
            MMT_LOG_TRACE("PTRACE_ATTACH succeeded for PID ", pid);

            // 2. SYNCHRONIZATION: The process doesn't stop instantly.
            // Using WNOHANG in a polling loop prevents the 'waitpid' deadlock
            // if the target is in 'D' (Uninterruptible Sleep) or a 'Zombie' state.
            int status;
            int attempts = mmt::PTRACE_ATTACH_MAX_ATTEMPTS;
            pid_t wait_res;

            MMT_LOG_TRACE("Waiting for PID ", pid, " to stop (max ", attempts, " attempts)");
            while ((wait_res = waitpid(pid, &status, WNOHANG)) == 0 && --attempts > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(mmt::PTRACE_ATTACH_POLL_INTERVAL_MS));
            }

            // Failure to stop within timeout: Clean up by detaching to avoid leaving the process trapped.
            if (wait_res <= 0 || !WIFSTOPPED(status)) {
                MMT_LOG_WARN("Process ", pid, " failed to stop within timeout (",
                            mmt::PTRACE_ATTACH_MAX_ATTEMPTS * mmt::PTRACE_ATTACH_POLL_INTERVAL_MS, "ms)");
                ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
                return std::unexpected(process_error{
                    .ec = make_error_code(process_errc::wait_failed),
                    .component = "WAIT_TIMEOUT_OR_FAILED",
                    .target_pid = pid
                });
            }
            MMT_LOG_TRACE("Process ", pid, " stopped successfully");

            // 3. POST-ATTACH VALIDATION: Anti-PID-Recycling Logic.
            // Between our scan and the 'attach', the original process might have died
            // and a NEW process might have taken its PID. We verify the actual PID
            // of the attached process using PTRACE_GETPID.
            errno = 0;  // Clear errno before call
            long traced_pid = ptrace(PTRACE_GETPID, pid, nullptr, nullptr);

            if (traced_pid < 0 || traced_pid != pid) {
                MMT_LOG_WARN("PID recycling detected: attached PID ", traced_pid, " != expected ", pid);
                // PID mismatch - this is a different process! Detach immediately.
                ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
                return std::unexpected(process_error{
                    .ec = make_error_code(process_errc::process_not_found),
                    .component = "PID_RECYCLING_DETECTED",
                    .target_pid = pid
                });
            }
            MMT_LOG_TRACE("PID validation passed: traced_pid == expected_pid == ", pid);

            // 4. SECONDARY VALIDATION: Verify /proc/[pid]/maps exists as additional check.
            // Using fs::path for safe path construction.
            fs::path maps_path = fs::path("/proc") / std::to_string(pid) / "maps";
            std::error_code ec;
            if (!fs::exists(maps_path, ec)) {
                MMT_LOG_DEBUG("Maps file not found for PID ", pid);
                ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
                return std::unexpected(process_error{
                    .ec = make_error_code(process_errc::process_not_found),
                    .component = "MAPS_VALIDATION",
                    .target_pid = pid
                });
            }

            MMT_LOG_INFO("Successfully attached to process ", pid);
            return ProcessAttachment(pid);
        }

        /**
         * @brief Destructor ensures the process is released.
         *
         * Crucial for system stability - without this, the analyzed process
         * stays frozen in stopped state.
         */
        ~ProcessAttachment() noexcept {
            if (is_active) {
                // PTRACE_DETACH allows the process to continue from where it stopped.
                // Note: ptrace does not throw exceptions
                ptrace(PTRACE_DETACH, target_pid, nullptr, nullptr);
            }
        }

        // Move-only semantics: prevent double-detaching by copying the PID state.
        ProcessAttachment(const ProcessAttachment&) = delete;
        ProcessAttachment& operator=(const ProcessAttachment&) = delete;

        ProcessAttachment(ProcessAttachment&& other) noexcept
            : target_pid(other.target_pid), is_active(other.is_active) {
            other.is_active = false; // Transfer ownership
        }

        /**
         * @brief Returns the target process ID.
         * @return PID of the attached process.
         */
        [[nodiscard]] int pid() const noexcept { return target_pid; }
    };
}
