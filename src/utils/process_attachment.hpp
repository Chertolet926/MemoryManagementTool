#pragma once

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <expected>
#include "common/errors.hpp"
#include <thread>

namespace utils {
    namespace fs = std::filesystem;

    /**
     * @brief RAII Guard for ptrace-based process suspension.
     * Ensures that the target process is ALWAYS resumed (detached) when the guard
     * goes out of scope, preventing processes from being stuck in "T" (Stopped) state.
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
         * @return Expected object containing the Guard or a detailed Error.
         */
        static std::expected<ProcessAttachment, process_error> attach(int pid) {
            
            // 1. ATTACH: Send SIGSTOP to the target via ptrace.
            // This is the first point of failure (e.g., EPERM if not root, ESRCH if process is gone).
            if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
                return std::unexpected(process_error{
                    .ec = boost::system::error_code(errno, boost::system::system_category()),
                    .component = "PTRACE_ATTACH",
                    .target_pid = pid
                });
            }

            // 2. SYNCHRONIZATION: The process doesn't stop instantly.
            // Using WNOHANG in a polling loop prevents the 'waitpid' deadlock 
            // if the target is in 'D' (Uninterruptible Sleep) or a 'Zombie' state.
            int status;
            int attempts = 50; // Approx 500ms total timeout
            pid_t wait_res;
            
            while ((wait_res = waitpid(pid, &status, WNOHANG)) == 0 && --attempts > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // Failure to stop within timeout: Clean up by detaching to avoid leaving the process trapped.
            if (wait_res <= 0 || !WIFSTOPPED(status)) {
                ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
                return std::unexpected(process_error{
                    .ec = make_error_code(process_errc::wait_failed),
                    .component = "WAIT_TIMEOUT_OR_FAILED",
                    .target_pid = pid
                });
            }

            // 3. POST-ATTACH VALIDATION: Anti-PID-Recycling Logic.
            // Between our scan and the 'attach', the original process might have died
            // and a NEW process might have taken its PID. We check for a vital procfs file
            // as a heartbeat to ensure the target is still valid/accessible.
            if (!fs::exists("/proc/" + std::to_string(pid) + "/maps")) {
                ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
                return std::unexpected(process_error{
                    .ec = make_error_code(process_errc::process_not_found),
                    .component = "RECYCLING_VALIDATION",
                    .target_pid = pid
                });
            }

            return ProcessAttachment(pid);
        }

        /**
         * @brief Destructor ensures the process is released.
         * Crucial for system stability; without this, the analyzed process stays frozen.
         */
        ~ProcessAttachment() {
            if (is_active) {
                // PTRACE_DETACH allows the process to continue from where it stopped.
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

        const int pid() const { return target_pid; }
    };
}
