#pragma once
/**
 * @file errors.hpp
 * @brief Common error handling module for the MMT codebase.
 *
 * This module provides unified error handling for both:
 *   1. Parser errors (dex namespace) - for parsing infrastructure
 *   2. Process/Utility errors (utils namespace) - for system operations
 *
 * To distinguish between error types:
 *   - Check error_code.category().name() == "dex_parser"  → Parser error
 *   - Check error_code.category().name() == "dex_process" → Process/Utility error
 *   - Check error_code.category() == get_parser_category() → Parser error
 *   - Check error_code.category() == get_process_category() → Process/Utility error
 */

#include <boost/system/error_code.hpp>
#include <boost/system/system_category.hpp>
#include <string_view>
#include <string>

namespace dex {

    /**
     * @enum parser_errc
     * @brief Parser error codes for Spirit X3 parsing operations.
     */
    enum class parser_errc {
        success = 0,           ///< Success (no error)
        invalid_syntax,        ///< Syntax error in input data
        incomplete_input       ///< Input data is incomplete or truncated
    };

    /**
     * @struct parser_category
     * @brief Error category for DEX parser errors.
     *
     * Integrates with boost::system to distinguish between OS errors and DEX parser errors.
     */
    struct parser_category : boost::system::error_category {

        /**
         * @brief Returns the unique identifier for this error category.
         * @return "dex_parser" string literal.
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
     * @brief Returns a reference to the static parser error category instance.
     * @return Reference to the singleton parser_category.
     *
     * Required for creating boost::system::error_code objects.
     */
    inline const boost::system::error_category& get_parser_category() {
        static parser_category instance;
        return instance;
    }

    /**
     * @brief Factory function to create error_code from parser_errc.
     * @param e The specific parser error code.
     * @return A complete error_code object containing the value and category.
     */
    inline boost::system::error_code make_error_code(parser_errc e) {
        return {static_cast<int>(e), get_parser_category()};
    }

    /**
     * @struct parser_error
     * @brief Main error reporting structure for parser failures.
     *
     * Contains all necessary information to diagnose parsing issues.
     */
    struct parser_error {
        boost::system::error_code ec;     ///< Error code (system or dex_parser category)
        std::string_view parser_name;     ///< Name of the parser where failure occurred
        size_t offset = 0;                ///< Byte offset in input where parsing failed
    };

} // namespace dex

/**
 * @namespace utils
 * @brief Utility functions and classes for process operations.
 */
namespace utils {

    /**
     * @enum process_errc
     * @brief Error codes for process and utility operations.
     */
    enum class process_errc {
        success = 0,           ///< Success (no error)
        not_a_pid,             ///< Directory name is not numeric
        access_denied,         ///< EPERM/EACCES - insufficient privileges
        process_not_found,     ///< ESRCH - target process does not exist
        attachment_busy,       ///< EBUSY - process is already being traced
        wait_failed,           ///< waitpid() returned an error
        invalid_proc_path      ///< The /proc filesystem is inaccessible
    };

    /**
     * @struct process_category
     * @brief Error category for process operations.
     */
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
     * @return Reference to the singleton process_category.
     */
    inline const boost::system::error_category& get_process_category() {
        static process_category instance;
        return instance;
    }

    /**
     * @brief Factory function to create error_code from process_errc.
     * @param e The specific process error code.
     * @return A complete error_code object containing the value and category.
     */
    inline boost::system::error_code make_error_code(process_errc e) {
        return {static_cast<int>(e), get_process_category()};
    }

    /**
     * @struct process_error
     * @brief Main error reporting structure for process operation failures.
     *
     * Contains all necessary information to diagnose process operation issues.
     */
    struct process_error {
        boost::system::error_code ec;       ///< Error code (system or dex_process category)
        std::string_view component;         ///< Component where failure occurred (e.g., "PTRACE_ATTACH")
        int target_pid = 0;                 ///< PID of target process (if applicable)
    };

} // namespace utils

/**
 * @section Boost.System Integration
 * @brief Registration of custom error codes with Boost.System.
 *
 * Enables automatic conversion between custom error codes and boost::system::error_code.
 */
namespace boost::system {
    /// Enable automatic conversion for parser_errc
    template <> struct is_error_code_enum<dex::parser_errc> : std::true_type {};
    /// Enable automatic conversion for process_errc
    template <> struct is_error_code_enum<utils::process_errc> : std::true_type {};
} // namespace boost::system
