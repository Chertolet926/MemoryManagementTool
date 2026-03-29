#pragma once

#include <boost/system/error_code.hpp>
#include <string>
#include <string_view>

namespace utils {

    enum class process_errc {
        success = 0,
        not_a_pid,          // Directory name is not numeric
        access_denied,      // EPERM/EACCES
        process_not_found,  // ESRCH
        attachment_busy,    // EBUSY
        wait_failed,        // waitpid error
        invalid_proc_path   // /proc missing
    };

    struct process_category : boost::system::error_category {
        const char* name() const noexcept override { return "dex_process"; }
        std::string message(int ev) const override {
            switch (static_cast<process_errc>(ev)) {
                case process_errc::not_a_pid:           return "Entry is not a valid PID directory";
                case process_errc::access_denied:       return "Access denied (insufficient privileges)";
                case process_errc::process_not_found:   return "Target process not found";
                case process_errc::attachment_busy:     return "Process is already being traced";
                case process_errc::wait_failed:         return "Failed to synchronize with process stop";
                case process_errc::invalid_proc_path:   return "The /proc filesystem is inaccessible";
                default:                                return "Unknown process error";
            }
        }
    };

    inline const boost::system::error_category& get_process_category() {
        static process_category instance;
        return instance;
    }

    inline boost::system::error_code make_error_code(process_errc e) {
        return {static_cast<int>(e), get_process_category()};
    }

    struct process_error {
        boost::system::error_code ec;
        std::string_view component;
        int target_pid = 0;
    };

} // namespace dex

namespace boost::system {
    template <> struct is_error_code_enum<utils::process_errc> : std::true_type {};
}