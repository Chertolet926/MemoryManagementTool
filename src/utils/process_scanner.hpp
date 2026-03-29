#pragma once
#include <vector>
#include <ranges>
#include "proc_utils.hpp"
#include "system_info.hpp"

namespace utils {
    namespace fs = std::filesystem; // Define alias at namespace level
    // Defines our filtering strategies
    enum class ScanMode { OnlyMe, AllHumanUsers, RootOnly, AllUserSpace };

    class ProcessScanner {
    public:
        struct Target { int pid; fs::path path; };

        /**
         * @brief Scans /proc using a declarative approach and KVParser.
         */
        static std::vector<Target> get_pids(ScanMode mode = ScanMode::OnlyMe) {
            std::vector<Target> results;
            std::error_code ec;
            auto it = fs::directory_iterator("/proc", ec);
            if (ec) return {};

            // Predicate capturing the REAL human user ID (even if under sudo)
            auto matches = [mode, my_uid = sys::get_real_user_id()](const fs::path& p) {
                if (!proc::is_user_land(p)) return false;
                
                uid_t p_uid = proc::get_uid(p);
                if (p_uid == static_cast<uid_t>(-1)) return false;

                switch (mode) {
                    case ScanMode::OnlyMe:        return p_uid == my_uid;
                    case ScanMode::AllHumanUsers: return p_uid >= 1000;
                    case ScanMode::RootOnly:      return p_uid == 0;
                    case ScanMode::AllUserSpace:  return true;
                    default: return false;
                }
            };

            // Process loop: Filter directories -> Extract PID -> Apply Mode Filter
            for (const auto& entry : it) {
                const auto& path = entry.path();
                if (!entry.is_directory()) continue;

                if (auto pid = proc::to_pid(path.filename().string()))
                    if (matches(path)) results.push_back({ *pid, path });
            }
            return results;
        }
    };
}
