#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>

// Assuming your headers are named as follows:
#include "utils/process_scanner.hpp"
#include "utils/process_attachment.hpp"

/**
 * @brief Helper to read the process command name safely.
 * @param path The filesystem path to /proc/[pid]
 */
std::string read_process_name(const std::filesystem::path& path) {
    std::ifstream comm_file(path / "comm");
    std::string name;
    if (std::getline(comm_file, name)) {
        return name;
    }
    return "<unknown>";
}

int main() {
    std::cout << std::left << std::setw(8) << "PID" 
              << std::setw(20) << "NAME" 
              << "STATUS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;

    // 1. SCAN: Get a snapshot of all user-land processes (skipping kernel threads)
    // The scanner handles /proc volatility and avoids exceptions.
    auto targets = utils::ProcessScanner::get_pids();

    for (const auto& target : targets) {
        // Skip the analyzer itself to avoid self-deadlock
        if (target.pid == getpid()) continue;

        std::cout << std::left << std::setw(8) << target.pid;

        // 2. ATTACH: Attempt to freeze the process safely.
        // This uses the RAII guard we built to prevent "stuck" processes.
        auto attachment = utils::ProcessAttachment::attach(target.pid);

        if (attachment) {
            // SUCCESS: The process is now SIGSTOPPED and under our control.
            // We can now read its internal state consistently.
            std::string name = read_process_name(target.path);
            
            std::cout << std::setw(20) << name 
                      << "[\033[32mATTACHED\033[0m]" << std::endl;

            /* 
               TODO: Place memory analysis logic here. 
               The process is frozen and won't change its memory layout.
            */

            // 'attachment' goes out of scope here: 
            // Destructor automatically calls PTRACE_DETACH.
        } else {
            // FAILURE: Identify why we couldn't attach.
            const auto& err = attachment.error();
            
            // We still try to read the name even if we can't attach (best effort)
            std::string name = read_process_name(target.path);

            std::cout << std::setw(20) << name 
                      << "[\033[31mSKIPPED\033[0m] (" << err.ec.message() << ")" << std::endl;
        }
    }

    std::cout << "\nAnalysis complete. All processes resumed." << std::endl;
    return 0;
}