#pragma once

#include <filesystem>
#include <algorithm>
#include <optional>
#include <charconv>
#include <ranges>

namespace utils {
    namespace fs = std::filesystem;

    /** 
     * @brief High-performance, exception-safe process scanner for Linux.
     * Automatically filters out kernel threads and invalid process entries.
     */
    class ProcessScanner {
    public:
        struct Target { 
            int pid;        
            fs::path path;  
        };

        /**
         * @brief Scans /proc and returns a list of valid User-land processes.
         * 
         * Logic:
         * 1. Iterate /proc using non-throwing std::error_code.
         * 2. Parse directory names to ensure they are numeric (PIDs).
         * 3. FILTER: Check for the existence of 'exe' link to skip Kernel Threads.
         * 
         * @return std::vector<Target> A snapshot of active user-space processes.
         */
        static std::vector<Target> get_pids() {
            std::vector<Target> results;
            std::error_code ec;

            // Initialize iterator with error_code to prevent exceptions if /proc access is restricted
            auto it = fs::directory_iterator("/proc", ec);
            if (ec) return results;

            for (; it != fs::end(it); it.increment(ec)) {
                // If a process exits during iteration, 'increment' captures the error gracefully
                if (ec) break;

                const auto& entry = *it;
                
                // Fast-path: Skip if entry is not a directory
                if (!entry.is_directory()) continue;

                const std::string name = entry.path().filename().string();
                int pid = 0;
                
                // High-performance integer parsing (no-copy, no-locale)
                auto [ptr, parse_ec] = std::from_chars(name.data(), name.data() + name.size(), pid);
                
                // Confirm the directory name is a valid numeric PID
                if (parse_ec == std::errc{} && ptr == name.data() + name.size()) {
                    
                    /**
                     * FILTER: KERNEL THREAD CHECK
                     * On Linux, kernel threads do not have an executable image.
                     * We check if the 'exe' symbolic link exists and is accessible.
                     * Using std::error_code here is crucial to avoid throws on permission errors.
                     */
                    std::error_code exe_ec;
                    auto exe_link = entry.path() / "exe";
                    
                    // If 'exe' does not exist or cannot be read, it's likely a kernel thread (or a zombie)
                    if (fs::exists(exe_link, exe_ec) && !exe_ec) {
                        results.push_back(Target{ pid, entry.path() });
                    }
                }
            }
            
            return results;
        }
    };
}
