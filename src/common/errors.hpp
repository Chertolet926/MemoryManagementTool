#pragma once

#include <boost/system/error_code.hpp>
#include <boost/system/system_category.hpp>
#include <string_view>
#include <string>

// ============================================================================
// COMMON ERROR HANDLING MODULE
// ============================================================================
// This module provides unified error handling for both:
//   1. Parser errors (dex namespace) - for parsing infrastructure
//   2. Process/Utility errors (utils namespace) - for system operations
//
// To distinguish between error types:
//   - Check error_code.category().name() == "dex_parser"  → Parser error
//   - Check error_code.category().name() == "dex_process" → Process/Utility error
//   - Check error_code.category() == get_parser_category() → Parser error
//   - Check error_code.category() == get_process_category() → Process/Utility error
// ============================================================================

namespace dex {

    // ========================================================================
    // PARSER ERROR CODES
    // ========================================================================
    /// Internal parser logic error codes
    enum class parser_errc {
        success = 0,
        invalid_syntax,    ///< Syntax error in input data
        incomplete_input   ///< Input data is incomplete or truncated
    };

    /// Definition of the error category for integration with boost::system
    /// This allows the system to distinguish between OS errors and DEX parser errors
    struct parser_category : boost::system::error_category {

        /**
         * @brief Returns the unique identifier for this error category.
         * This allows the system to distinguish between OS errors and DEX parser errors.
         * @return A string literal "dex_parser".
         */
        const char* name() const noexcept override { return "dex_parser"; };

        /**
         * @brief Converts a numeric error value into a human-readable string.
         * @param ev The integer representation of a parser_errc.
         * @return A descriptive error message.
         */
        std::string message(int ev) const override {
            switch (static_cast<parser_errc>(ev)) {
            case parser_errc::invalid_syntax:
                return "Invalid syntax";
            case parser_errc::incomplete_input:
                return "Incomplete input";
            default:
                return "Unknown parser error";
            }
        }
    };

    /**
     * @brief Returns a reference to the static dex error category instance.
     * Required for creating boost::system::error_code objects.
     * @return A reference to the singleton parser_category.
     */
    inline const boost::system::error_category& get_parser_category() {
        static parser_category instance;
        return instance;
    }

    /**
     * @brief Factory function to create a boost::system::error_code from a parser_errc.
     * @param e The specific parser error code.
     * @return A complete error_code object containing the value and the dex category.
     */
    inline boost::system::error_code make_error_code(parser_errc e) {
        return {static_cast<int>(e), get_parser_category()};
    }

    /**
     * @brief Main error reporting structure for parser failures.
     * Contains all necessary information to diagnose parsing issues.
     */
    struct parser_error {
        boost::system::error_code ec;     ///< The error code (either OS system error or dex_parser error)
        std::string_view parser_name;     ///< The name of the specific parser where the failure occurred (e.g., "CPU_PARSER")
        size_t offset = 0;                ///< Byte offset in the input stream where parsing failed
    };

} // namespace dex

// ============================================================================
// UTILITY / PROCESS ERROR CODES
// ============================================================================
namespace utils {

    /// Process and utility operation error codes
    enum class process_errc {
        success = 0,
        not_a_pid,          ///< Directory name is not numeric
        access_denied,      ///< EPERM/EACCES - insufficient privileges
        process_not_found,  ///< ESRCH - target process does not exist
        attachment_busy,    ///< EBUSY - process is already being traced
        wait_failed,        ///< waitpid() returned an error
        invalid_proc_path   ///< The /proc filesystem is inaccessible
    };

    /// Definition of the error category for process operations
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

    /**
     * @brief Returns a reference to the static process error category instance.
     * @return A reference to the singleton process_category.
     */
    inline const boost::system::error_category& get_process_category() {
        static process_category instance;
        return instance;
    }

    /**
     * @brief Factory function to create a boost::system::error_code from a process_errc.
     * @param e The specific process error code.
     * @return A complete error_code object containing the value and the process category.
     */
    inline boost::system::error_code make_error_code(process_errc e) {
        return {static_cast<int>(e), get_process_category()};
    }

    /**
     * @brief Main error reporting structure for process operation failures.
     * Contains all necessary information to diagnose process operation issues.
     */
    struct process_error {
        boost::system::error_code ec;       ///< The error code (either OS system error or dex_process error)
        std::string_view component;         ///< The component where the failure occurred (e.g., "PTRACE_ATTACH")
        int target_pid = 0;                 ///< The PID of the target process (if applicable)
    };

} // namespace utils

// ============================================================================
// BOOST.SYSTEM INTEGRATION
// ============================================================================
/// Registration with Boost.System to enable automatic error code conversions
namespace boost::system {
    /// Enable automatic conversion for parser_errc
    template <> struct is_error_code_enum<dex::parser_errc> : std::true_type {};
    /// Enable automatic conversion for process_errc
    template <> struct is_error_code_enum<utils::process_errc> : std::true_type {};
} // namespace boost::system
