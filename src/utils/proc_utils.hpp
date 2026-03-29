#pragma once

#include <filesystem>
#include <optional>
#include <charconv>

// Integrating your existing parser infrastructure
#include "dex/kv_parser.hpp" 
#include "utils/system_info.hpp"

namespace utils::proc {
    namespace fs = std::filesystem;

    /** 
     * @brief Extracts UID from /proc/[pid]/status using the existing KVParser.
     * @param p The path to the process directory (e.g., /proc/1234)
     * @return The Real UID (ruid) or -1 on failure.
     */
    inline uid_t get_uid(const fs::path& p) {
        // Use the existing KVParser to parse the 'status' file into a KVRegistry
        auto result = dex::KVParser::from_file(p / "status");
        
        if (result) {
            // Locate the "Uid" key in the registry
            auto it = result->find("Uid");
            if (it != result->end() && !it->second.empty()) {
                /** 
                 * According to proc(5), the 'Uid' line contains 4 values: 
                 * Real, Effective, Saved, and File System UIDs.
                 * We extract the first one (Real UID).
                 */
                const auto& val = it->second[0];
                
                // If it's stored as a long/int in your Value variant
                if (auto* id = std::get_if<long>(&val)) return static_cast<uid_t>(*id);
                
                // Fallback: if it's stored as a string, parse it
                if (auto* s = std::get_if<std::string>(&val)) {
                    uid_t ruid;
                    auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), ruid);
                    if (ec == std::errc{}) return ruid;
                }
            }
        }
        return -1; // Return invalid UID on parse error or missing field
    }

    /** @brief Fast PID parsing using std::from_chars (No-copy/No-throw). */
    inline std::optional<int> to_pid(const std::string& s) {
        int pid;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), pid);
        return (ec == std::errc{} && ptr == s.data() + s.size()) ? std::optional(pid) : std::nullopt;
    }

    /** @brief Determines if a process is User-land by checking the 'exe' symlink. */
    inline bool is_user_land(const fs::path& p) {
        std::error_code ec;
        return fs::exists(p / "exe", ec);
    }
}
