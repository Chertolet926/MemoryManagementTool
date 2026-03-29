#include <gtest/gtest.h>
#include "utils/system_info.hpp"
#include "utils/proc_utils.hpp"
#include "utils/process_scanner.hpp"
#include <cstdlib>
#include <unistd.h>

// ============================================================================
// SYSTEM INFO TESTS (SUDO DETECTION & UID PARSING)
// ============================================================================

/**
 * @brief Tests that is_running_under_sudo() doesn't crash with malformed env.
 */
TEST(SystemInfoTest, SudoDetectionNoCrash) {
    // This test should never crash, even with malformed environment
    EXPECT_NO_THROW((void)utils::sys::is_running_under_sudo());
}

/**
 * @brief Tests parse_uid_safe with valid input.
 */
TEST(SystemInfoTest, ParseUidSafeValid) {
    auto result = utils::sys::parse_uid_safe("1000");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1000);
}

/**
 * @brief Tests parse_uid_safe with null pointer.
 */
TEST(SystemInfoTest, ParseUidSafeNull) {
    auto result = utils::sys::parse_uid_safe(nullptr);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().component, "UID_PARSE");
}

/**
 * @brief Tests parse_uid_safe with empty string.
 */
TEST(SystemInfoTest, ParseUidSafeEmpty) {
    auto result = utils::sys::parse_uid_safe("");
    ASSERT_FALSE(result.has_value());
}

/**
 * @brief Tests parse_uid_safe with non-numeric input.
 */
TEST(SystemInfoTest, ParseUidSafeNonNumeric) {
    auto result = utils::sys::parse_uid_safe("abc");
    ASSERT_FALSE(result.has_value());
}

/**
 * @brief Tests parse_uid_safe with mixed input.
 */
TEST(SystemInfoTest, ParseUidSafeMixed) {
    auto result = utils::sys::parse_uid_safe("1000abc");
    ASSERT_FALSE(result.has_value());
}

/**
 * @brief Tests get_real_user_id_safe returns a valid UID.
 */
TEST(SystemInfoTest, GetRealUserIdSafe) {
    auto result = utils::sys::get_real_user_id_safe();
    ASSERT_TRUE(result.has_value());
    // UID 0 is root, > 0 is regular user - both are valid
    EXPECT_GE(result.value(), 0);
}

// ============================================================================
// PROC UTILS TESTS (GET_UID ERROR HANDLING)
// ============================================================================

/**
 * @brief Tests get_uid with non-existent path.
 */
TEST(ProcUtilsTest, GetUidNonExistentPath) {
    auto result = utils::proc::get_uid("/proc/999999999");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().component, "GET_UID");
}

/**
 * @brief Tests get_uid with invalid path (not a process).
 */
TEST(ProcUtilsTest, GetUidInvalidPath) {
    auto result = utils::proc::get_uid("/nonexistent/path");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().component, "GET_UID");
}

/**
 * @brief Tests get_uid with current process (should succeed).
 */
TEST(ProcUtilsTest, GetUidCurrentProcess) {
    std::string path = "/proc/" + std::to_string(getpid());
    auto result = utils::proc::get_uid(path);
    
    // This should succeed for the current process
    ASSERT_TRUE(result.has_value()) << "Failed to get UID of current process: " 
                                     << result.error().ec.message();
    EXPECT_EQ(result.value(), getuid());
}

// ============================================================================
// PROCESS SCANNER TESTS (SCAN WITH ERROR REPORTING)
// ============================================================================

/**
 * @brief Tests that scan() returns a valid result structure.
 */
TEST(ProcessScannerTest, ScanReturnsValidResult) {
    auto result = utils::ProcessScanner::scan(utils::ScanMode::AllUserSpace);
    
    ASSERT_TRUE(result.has_value()) << "Scan failed: " << result.error().ec.message();
    // Should find at least the current process
    EXPECT_GE(result->count(), 1);
}

/**
 * @brief Tests scan with OnlyMe mode.
 */
TEST(ProcessScannerTest, ScanOnlyMe) {
    auto result = utils::ProcessScanner::scan(utils::ScanMode::OnlyMe);
    
    ASSERT_TRUE(result.has_value());
    // Should find at least the current process
    EXPECT_GE(result->count(), 1);
    
    // All matched processes should belong to current user
    uid_t my_uid = getuid();
    for (const auto& path : result->matched_paths) {
        auto uid_result = utils::proc::get_uid(path);
        ASSERT_TRUE(uid_result.has_value());
        EXPECT_EQ(uid_result.value(), my_uid);
    }
}

/**
 * @brief Tests that skipped processes are tracked.
 */
TEST(ProcessScannerTest, ScanTracksSkipped) {
    auto result = utils::ProcessScanner::scan(utils::ScanMode::AllUserSpace);
    
    ASSERT_TRUE(result.has_value());
    // Just verify the structure is correct
    EXPECT_GE(result->count(), 0);
    EXPECT_GE(result->skipped_count(), 0);
}

/**
 * @brief Tests deprecated get_pids() still works for backward compatibility.
 */
TEST(ProcessScannerTest, GetPidsBackwardCompatibility) {
    auto results = utils::ProcessScanner::get_pids(utils::ScanMode::AllUserSpace);
    
    // Should find at least the current process
    EXPECT_GE(results.size(), 1);
}

// ============================================================================
// ERROR CODE TESTS
// ============================================================================

/**
 * @brief Tests that proc_errc messages are properly defined.
 */
TEST(ProcErrorCodeTest, ProcErrcMessages) {
    using namespace utils::proc;
    
    EXPECT_STREQ(make_error_code(proc_errc::file_not_found).message().c_str(),
                 "Process status file not found");
    EXPECT_STREQ(make_error_code(proc_errc::permission_denied).message().c_str(),
                 "Permission denied reading status");
    EXPECT_STREQ(make_error_code(proc_errc::parse_error).message().c_str(),
                 "Failed to parse UID value");
}

/**
 * @brief Tests that process_errc messages are properly defined.
 */
TEST(ProcErrorCodeTest, ProcessErrcMessages) {
    using namespace utils;
    
    EXPECT_STREQ(make_error_code(process_errc::not_a_pid).message().c_str(),
                 "Entry is not a valid PID directory");
    EXPECT_STREQ(make_error_code(process_errc::access_denied).message().c_str(),
                 "Access denied (insufficient privileges)");
}
