/**
 * @file main.cpp
 * @brief Process Scanner Integration Test application.
 *
 * This is a test application that demonstrates the ProcessScanner functionality
 * by running scans with different filter modes and displaying results.
 */

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "utils/process_scanner.hpp"
#include "common/logger.hpp"

/**
 * @brief Helper function to display test results in a table format.
 * @param mode Scan mode to test.
 * @param label Human-readable label for the mode.
 */
void test_mode(utils::ScanMode mode, const std::string& label) {
    MMT_LOG_DEBUG("Running scan mode: ", label);
    
    auto result = utils::ProcessScanner::scan(mode);

    std::cout << std::left << std::setw(18) << label;

    if (!result) {
        std::cout << ": [ERROR: " << result.error().ec.message() << "]";
        MMT_LOG_ERROR("Scan failed for mode ", label, ": ", result.error().ec.message());
    } else {
        std::cout << ": [" << std::setw(4) << result->count() << " found]";

        if (!result->matched_paths.empty()) {
            // Extract PID from first matched path
            auto path = result->matched_paths[0];
            std::string pid_str = path.filename().string();
            int pid = 0;
            std::from_chars(pid_str.data(), pid_str.data() + pid_str.size(), pid);
            std::cout << " (Sample PID: " << pid << ")";
        }

        if (result->skipped_count() > 0) {
            std::cout << " [" << result->skipped_count() << " skipped]";
        }
    }
    std::cout << std::endl;
}

/**
 * @brief Main entry point for the Process Scanner Integration Test.
 * @return 0 on success, non-zero on failure.
 *
 * Runs process scans with different filter modes:
 * - OnlyMe: processes owned by current user
 * - AllHumanUsers: processes with UID >= 1000
 * - RootOnly: processes owned by root (UID 0)
 * - AllUserSpace: all user-space processes
 */
int main() {
    // Initialize logger from environment
    const char* log_level = std::getenv("MMT_LOG_LEVEL");
    if (log_level) {
        MMT_LOG_INFO("Log level set to: ", log_level);
    } else {
        MMT_LOG_INFO("Log level not set. Use MMT_LOG_LEVEL=debug for verbose output.");
    }
    
    // Context Info
    uid_t effective = getuid();
    uid_t real = utils::sys::get_real_user_id_safe().value_or(getuid());

    std::cout << "=== Process Scanner Integration Test ===\n";
    std::cout << "Running as (EUID): " << effective << (effective == 0 ? " (ROOT)" : " (USER)") << "\n";
    std::cout << "Real Human (RUID): " << real << "\n";
    std::cout << "Sudo detected: " << (utils::sys::is_running_under_sudo() ? "YES" : "NO") << "\n";
    std::cout << std::string(45, '-') << "\n";

    MMT_LOG_INFO("Starting process scanner tests");

    // Run all filter modes
    test_mode(utils::ScanMode::OnlyMe,         "ONLY_ME");
    test_mode(utils::ScanMode::AllHumanUsers,  "ALL_HUMAN_USERS");
    test_mode(utils::ScanMode::RootOnly,       "ROOT_ONLY");
    test_mode(utils::ScanMode::AllUserSpace,   "ALL_USER_SPACE");

    std::cout << std::string(45, '-') << "\n";

    if (effective == 0 && real != 0) {
        std::cout << "Note: ONLY_ME and ROOT_ONLY correctly show different counts\n";
        std::cout << "because the scanner detected you are using 'sudo'.\n";
        MMT_LOG_DEBUG("Running under sudo: effective=", effective, ", real=", real);
    }

    MMT_LOG_INFO("Process scanner tests completed");
    return 0;
}
