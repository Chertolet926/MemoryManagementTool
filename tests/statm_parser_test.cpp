#include <gtest/gtest.h>
#include "../src/dex/statm_parser.hpp" // Path to your header

namespace dex::test {

/** @brief Test suite for /proc/[pid]/statm parsing. */
TEST(StatmParserTest, ParsesStandardOutput) {
    // Values provided: size, resident, shared, text, lib, data, dt
    std::string_view input = "10044 5409 2849 18 0 4914 0";
    
    auto res = StatmParser::from_string(input);
    
    // 1. Verify the result is valid
    ASSERT_TRUE(res.has_value()) << "Parser failed to process valid statm input";
    
    // 2. Verify each field according to the Linux statm documentation
    const auto& entry = res.value();
    EXPECT_EQ(entry.size,     10044); // Total program size (pages)
    EXPECT_EQ(entry.resident, 5409);  // Resident set size
    EXPECT_EQ(entry.shared,   2849);  // Shared pages
    EXPECT_EQ(entry.text,     18);    // Text (code)
    EXPECT_EQ(entry.lib,      0);     // Library (unused, always 0)
    EXPECT_EQ(entry.data,     4914);  // Data + stack
    EXPECT_EQ(entry.dt,       0);     // Dirty pages (unused, always 0)
}

/** @brief Verifies the parser handles varying amounts of whitespace (tabs/spaces). */
TEST(StatmParserTest, HandlesIrregularWhitespace) {
    // Standard kernel output: values separated by spaces/tabs on one line.
    std::string_view input = "100\t200   300  400 500 600 700";
    
    auto res = StatmParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size, 100);
    EXPECT_EQ(res->dt,   700);
}

/** @brief Negative test: Fails if there are fewer than 7 integers. */
TEST(StatmParserTest, FailsOnIncompleteInput) {
    std::string_view input = "1 2 3 4 5 6"; // Missing 7th digit
    
    auto res = StatmParser::from_string(input);
    
    EXPECT_FALSE(res.has_value());
    // Verification: invalid_syntax error code is expected from BaseParser
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
}

} // namespace dex::test
