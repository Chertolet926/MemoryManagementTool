#include <iostream>
#include <iomanip>
#include "utils/process_scanner.hpp"

/** @brief Helper to display test results in a table format */
void test_mode(utils::ScanMode mode, const std::string& label) {
    auto results = utils::ProcessScanner::get_pids(mode);
    
    std::cout << std::left << std::setw(18) << label 
              << ": [" << std::setw(4) << results.size() << " found]";
    
    if (!results.empty()) {
        std::cout << " (Sample PID: " << results[0].pid << ")";
    }
    std::cout << std::endl;
}

int main() {
    // Context Info
    uid_t effective = getuid();
    uid_t real = utils::sys::get_real_user_id();

    std::cout << "=== Process Scanner Integration Test ===\n";
    std::cout << "Running as (EUID): " << effective << (effective == 0 ? " (ROOT)" : " (USER)") << "\n";
    std::cout << "Real Human (RUID): " << real << "\n";
    std::cout << std::string(45, '-') << "\n";

    // Run all filter modes
    test_mode(utils::ScanMode::OnlyMe,         "ONLY_ME");
    test_mode(utils::ScanMode::AllHumanUsers,  "ALL_HUMAN_USERS");
    test_mode(utils::ScanMode::RootOnly,       "ROOT_ONLY");
    test_mode(utils::ScanMode::AllUserSpace,   "ALL_USER_SPACE");

    std::cout << std::string(45, '-') << "\n";
    
    if (effective == 0 && real != 0) {
        std::cout << "Note: ONLY_ME and ROOT_ONLY correctly show different counts\n";
        std::cout << "because the scanner detected you are using 'sudo'.\n";
    }

    return 0;
}
