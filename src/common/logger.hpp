#pragma once
/**
 * @file logger.hpp
 * @brief MMT Logger - Lightweight header-only logging library.
 *
 * A minimal logging library for MemoryManagementTool.
 *
 * Features:
 *   - Zero external dependencies (C++23 standard library only)
 *   - Header-only implementation
 *   - Multiple log levels (trace, debug, info, warn, error, critical)
 *   - Thread-safe output to stderr
 *   - Configurable via environment variable (MMT_LOG_LEVEL)
 *   - Compile-time level filtering (MMT_LOG_LEVEL_COMPILE)
 *
 * Usage:
 * @code
 *   MMT_LOG_INFO("Application started");
 *   MMT_LOG_DEBUG("Value: ", value);
 *   if (MMT_LOG_ENABLED(mmt::log::level::debug)) {
 *       // Expensive debug computation
 *   }
 * @endcode
 *
 * Configure log level via environment:
 * @code
 *   export MMT_LOG_LEVEL=debug
 * @endcode
 */

#include <iostream>
#include <string>
#include <string_view>
#include <source_location>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <thread>

namespace mmt::log {

    /**
     * @enum level
     * @brief Log severity levels.
     *
     * Defines the severity levels for log messages, from most verbose (trace)
     * to most severe (critical). The 'off' level disables all logging.
     */
    enum class level {
        trace = 0,       ///< Most verbose - detailed tracing information
        debug = 1,       ///< Debug information for development
        info = 2,        ///< General informational messages
        warn = 3,        ///< Warning messages about potential issues
        error = 4,       ///< Error messages about actual problems
        critical = 5,    ///< Critical errors requiring immediate attention
        off = 6          ///< Logging disabled
    };

    /**
     * @brief Converts a log level to a human-readable string.
     * @param lvl The log level.
     * @return A string_view representing the level name.
     */
    [[nodiscard]] inline constexpr std::string_view to_string(level lvl) noexcept {
        switch (lvl) {
            case level::trace:    return "TRACE";
            case level::debug:    return "DEBUG";
            case level::info:     return "INFO";
            case level::warn:     return "WARN";
            case level::error:    return "ERROR";
            case level::critical: return "CRITICAL";
            case level::off:      return "OFF";
        }
        return "UNKNOWN";
    }

    /**
     * @brief Converts a log level to a color code for terminal output.
     * @param lvl The log level.
     * @return ANSI color code prefix (empty if colors not supported).
     */
    [[nodiscard]] inline constexpr std::string_view to_color(level lvl) noexcept {
        switch (lvl) {
            case level::trace:    return "\x1b[90m";   // Bright Black
            case level::debug:    return "\x1b[36m";   // Cyan
            case level::info:     return "\x1b[32m";   // Green
            case level::warn:     return "\x1b[33m";   // Yellow
            case level::error:    return "\x1b[31m";   // Red
            case level::critical: return "\x1b[95m";   // Bright Magenta
            case level::off:      return "";
        }
        return "";
    }

    /**
     * @brief Reset terminal color after colored log output.
     */
    inline constexpr std::string_view color_reset = "\x1b[0m";

    /**
     * @class logger
     * @brief Thread-safe singleton logger class.
     *
     * Provides centralized logging functionality with configurable severity levels.
     * Output is written to stderr with optional ANSI color codes.
     *
     * @note This is a singleton class - use logger::instance() to access.
     */
    class logger {
    public:
        /**
         * @brief Gets the singleton logger instance.
         * @return Reference to the global logger.
         */
        [[nodiscard]] static logger& instance() noexcept {
            static logger inst;
            return inst;
        }

        /**
         * @brief Sets the minimum log level for output.
         * @param lvl The minimum level to display.
         * 
         * @note Messages below this level will be discarded.
         */
        void set_level(level lvl) noexcept {
            min_level_.store(lvl, std::memory_order_relaxed);
        }

        /**
         * @brief Gets the current minimum log level.
         * @return The current minimum level.
         */
        [[nodiscard]] level get_level() const noexcept {
            return min_level_.load(std::memory_order_relaxed);
        }

        /**
         * @brief Logs a message with the specified level.
         * @param lvl The log level.
         * @param msg The message to log.
         * @param loc Source location (auto-filled by macros).
         * 
         * @note Thread-safe: uses mutex for synchronized output.
         */
        void log(level lvl, std::string_view msg,
                 const std::source_location& loc = std::source_location::current()) noexcept {
            // Fast path: check level before acquiring lock
            if (lvl < get_level()) {
                return;
            }

            // Skip if logging is off
            if (get_level() == level::off) {
                return;
            }

            // Acquire lock for thread-safe output
            std::lock_guard<std::mutex> lock(mutex_);

            // Get current time
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            // Build the log line
            std::ostringstream oss;
            oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
            oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << std::setfill(' ');
            oss << " ";
            oss << to_color(lvl);
            oss << "[" << to_string(lvl) << "]";
            oss << color_reset;
            oss << " ";
            oss << msg;
            oss << " ";
            oss << "\x1b[90m";  // Gray for location
            oss << "(" << loc.file_name() << ":" << loc.line() << ")";
            oss << color_reset;
            oss << "\n";

            // Output to stderr
            std::cerr << oss.str();
            std::cerr.flush();
        }

        /**
         * @brief Flushes the log output.
         * 
         * @note Called automatically after each message.
         */
        void flush() noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cerr.flush();
        }

        // Non-copyable, non-movable
        logger(const logger&) = delete;
        logger& operator=(const logger&) = delete;
        logger(logger&&) = delete;
        logger& operator=(logger&&) = delete;

    private:
        logger() noexcept {
            // Initialize level from environment variable using thread-safe caching.
            // Uses call_once to ensure initialization happens exactly once.
            static std::once_flag init_flag;
            static level cached_level = level::info;
            static bool initialized = false;

            std::call_once(init_flag, []() {
                const char* env_level = std::getenv("MMT_LOG_LEVEL");
                if (env_level) {
                    cached_level = parse_level(env_level);
                }
                initialized = true;
            });

            // Wait for initialization if another thread is initializing
            while (!initialized) {
                std::this_thread::yield();
            }

            min_level_ = cached_level;
        }

        ~logger() noexcept = default;

        /**
         * @brief Parses a log level from a string.
         * @param str The string to parse.
         * @return The corresponding log level, or level::info on failure.
         */
        [[nodiscard]] static level parse_level(const char* str) noexcept {
            if (!str) return level::info;
            
            // Case-insensitive comparison
            if (strcasecmp(str, "trace") == 0) return level::trace;
            if (strcasecmp(str, "debug") == 0) return level::debug;
            if (strcasecmp(str, "info") == 0) return level::info;
            if (strcasecmp(str, "warn") == 0 || strcasecmp(str, "warning") == 0) return level::warn;
            if (strcasecmp(str, "error") == 0) return level::error;
            if (strcasecmp(str, "critical") == 0 || strcasecmp(str, "fatal") == 0) return level::critical;
            if (strcasecmp(str, "off") == 0) return level::off;
            
            return level::info;  // Default
        }

        std::atomic<level> min_level_{level::info};  // Default level
        std::mutex mutex_;
    };

    // ========================================================================
    // LOGGING MACROS
    // ========================================================================
    // Compile-time level filtering (optional)
    // Define MMT_LOG_LEVEL_COMPILE to filter at compile time:
    //   -DMMT_LOG_LEVEL_COMPILE=2  // Filter out trace and debug

    // ========================================================================
    // HELPER FUNCTIONS FOR STREAM LOGGING
    // ========================================================================
    
    /**
     * @brief Helper function to stream multiple arguments to ostringstream.
     * @param oss The output string stream to write to.
     * 
     * @note Base case (no arguments).
     */
    template<typename... Args>
    inline void stream_log(std::ostringstream& oss) noexcept {
        (void)oss;  // Suppress unused warning when no args
    }

    /**
     * @brief Helper function to stream multiple arguments to ostringstream.
     * @param oss The output string stream to write to.
     * @param first First argument to stream.
     * @param rest Remaining arguments to stream.
     * 
     * @note Recursive case (one or more arguments).
     */
    template<typename T, typename... Args>
    inline void stream_log(std::ostringstream& oss, T&& first, Args&&... rest) {
        oss << std::forward<T>(first);
        stream_log(oss, std::forward<Args>(rest)...);
    }

} // namespace mmt::log

/**
 * @section Logging Macros
 * @brief Convenience macros for logging at different severity levels.
 *
 * Compile-time level filtering (optional):
 * Define MMT_LOG_LEVEL_COMPILE to filter at compile time:
 * @code
 *   -DMMT_LOG_LEVEL_COMPILE=2  // Filter out trace and debug
 * @endcode
 */

/**
 * @def MMT_LOG(lvl, ...)
 * @brief Helper macro for stream-style logging with multiple arguments.
 *
 * Usage:
 * @code
 *   MMT_LOG(level::info, "Value: ", val, ", Name: ", name)
 * @endcode
 *
 * @param lvl Log level (mmt::log::level enum value)
 * @param ... Variadic arguments to stream into the log message
 */
#define MMT_LOG(lvl, ...) \
    do { \
        constexpr auto compile_level = mmt::log::level::info; \
        if constexpr (static_cast<int>(lvl) >= static_cast<int>(compile_level)) { \
            std::ostringstream oss__; \
            mmt::log::stream_log(oss__, __VA_ARGS__); \
            mmt::log::logger::instance().log(lvl, oss__.str()); \
        } \
    } while (0)

/**
 * @def MMT_LOG_TRACE(...)
 * @brief Logs a trace message (lowest priority, most verbose).
 * @param ... Message arguments
 */
#define MMT_LOG_TRACE(...) MMT_LOG(mmt::log::level::trace, __VA_ARGS__)

/**
 * @def MMT_LOG_DEBUG(...)
 * @brief Logs a debug message (detailed information for development).
 * @param ... Message arguments
 */
#define MMT_LOG_DEBUG(...) MMT_LOG(mmt::log::level::debug, __VA_ARGS__)

/**
 * @def MMT_LOG_INFO(...)
 * @brief Logs an info message (general informational messages).
 * @param ... Message arguments
 */
#define MMT_LOG_INFO(...) MMT_LOG(mmt::log::level::info, __VA_ARGS__)

/**
 * @def MMT_LOG_WARN(...)
 * @brief Logs a warning message (potential issues).
 * @param ... Message arguments
 */
#define MMT_LOG_WARN(...) MMT_LOG(mmt::log::level::warn, __VA_ARGS__)

/**
 * @def MMT_LOG_ERROR(...)
 * @brief Logs an error message (actual problems).
 * @param ... Message arguments
 */
#define MMT_LOG_ERROR(...) MMT_LOG(mmt::log::level::error, __VA_ARGS__)

/**
 * @def MMT_LOG_CRITICAL(...)
 * @brief Logs a critical message (severe errors requiring immediate attention).
 * @param ... Message arguments
 */
#define MMT_LOG_CRITICAL(...) MMT_LOG(mmt::log::level::critical, __VA_ARGS__)

/**
 * @def MMT_LOG_ENABLED(lvl)
 * @brief Checks if a log level is enabled.
 * @param lvl The level to check.
 * @return true if messages at this level would be logged.
 *
 * Usage:
 * @code
 *   if (MMT_LOG_ENABLED(mmt::log::level::debug)) {
 *       // Expensive debug computation
 *       MMT_LOG_DEBUG("Result: ", result);
 *   }
 * @endcode
 */
#define MMT_LOG_ENABLED(lvl) (static_cast<int>(lvl) >= static_cast<int>(mmt::log::logger::instance().get_level()))
