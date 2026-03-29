#pragma once
/**
 * @file constants.hpp
 * @brief Centralized constants for the MMT codebase.
 *
 * This file provides commonly used constants to avoid duplication
 * across the codebase. Include this file instead of defining constants locally.
 */

#include <sys/types.h>
#include <cstddef>

namespace mmt {

    /**
     * @name PID Constants
     * @brief Constants related to Process ID validation.
     */
    ///@{

    /**
     * @brief Minimum valid PID value.
     * @note PID 0 is reserved for the kernel.
     */
    inline constexpr pid_t MIN_VALID_PID = 1;

    /**
     * @brief Maximum valid PID value.
     * @note Linux default: /proc/sys/kernel/pid_max (4194303)
     */
    inline constexpr pid_t MAX_VALID_PID = 4194303;

    ///@}

    /**
     * @name UID Constants
     * @brief Constants related to User ID validation.
     */
    ///@{

    /**
     * @brief Minimum UID for human users.
     * @note System users typically have UID < 1000.
     */
    inline constexpr uid_t MIN_HUMAN_USER_UID = 1000;

    /**
     * @brief Root UID.
     */
    inline constexpr uid_t ROOT_UID = 0;

    ///@}

    /**
     * @name PTRACE Constants
     * @brief Constants for ptrace-based process attachment.
     */
    ///@{

    /**
     * @brief Maximum number of polling attempts when waiting for process to stop.
     */
    inline constexpr int PTRACE_ATTACH_MAX_ATTEMPTS = 50;

    /**
     * @brief Interval between polling attempts in milliseconds.
     */
    inline constexpr int PTRACE_ATTACH_POLL_INTERVAL_MS = 10;

    ///@}

    /**
     * @name Path Constants
     * @brief Constants for filesystem path operations.
     */
    ///@{

    /**
     * @brief Maximum path length for reading /proc/[pid]/exe symlink target.
     */
    inline constexpr std::size_t MAX_EXE_PATH = 4096;

    ///@}

} // namespace mmt
