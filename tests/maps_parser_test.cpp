#include "../src/dex/maps_parser.hpp"
#include <gtest/gtest.h>

/**
 * @brief Basic success scenario for a single standard /proc/maps entry.
 */
TEST(MapsParserTest, BasicSuccess) {
    std::string_view input = "7f7f3a200000-7f7f3a400000 r-xp 00001000 08:01 123456 /usr/lib/libc.so";
    auto res = dex::MapsParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 1);
    
    const auto& entry = res->at(0);
    EXPECT_EQ(entry.start, 0x7f7f3a200000);
    EXPECT_EQ(entry.end, 0x7f7f3a400000);
    EXPECT_EQ(entry.offset, 0x1000);
    EXPECT_EQ(entry.dev, "08:01");
    EXPECT_EQ(entry.inode, 123456);
    EXPECT_EQ(entry.path, "/usr/lib/libc.so");

    // Verify bitfield conversion
    EXPECT_TRUE(entry.perms.is_r());
    EXPECT_FALSE(entry.perms.is_w());
    EXPECT_TRUE(entry.perms.is_x());
    EXPECT_TRUE(entry.perms.is_private());
}

/**
 * @brief Verifies handling of multiple lines and anonymous (empty path) mappings.
 */
TEST(MapsParserTest, MultiLineAndAnonymous) {
    std::string_view input = 
        "55a2b0c00000-55a2b0c01000 r--p 00000000 08:01 500 /bin/app\n"
        "55a2b0c01000-55a2b0c02000 rw-p 00000000 00:00 0 "; // Anonymous mapping
    
    auto res = dex::MapsParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2);
    
    EXPECT_EQ(res->at(0).path, "/bin/app");
    EXPECT_TRUE(res->at(1).path.empty());
    EXPECT_EQ(res->at(1).size(), 0x1000); // Testing the helper method
}

/**
 * @brief Tests special kernel tags and shared memory flag.
 */
TEST(MapsParserTest, KernelTagsAndSharedFlag) {
    std::string_view input = 
        "7ffdf8000000-7ffdf8021000 rw-p 00000000 00:00 0 [stack]\n"
        "7f0000000000-7f0000001000 rw-s 00000000 00:01 999 /dev/shm/tmp";
    
    auto res = dex::MapsParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    
    // Check [stack]
    EXPECT_EQ(res->at(0).path, "[stack]");
    EXPECT_TRUE(res->at(0).perms.is_w());
    
    // Check Shared flag 's'
    EXPECT_TRUE(res->at(1).perms.shared);
    EXPECT_FALSE(res->at(1).perms.is_private());
}

/**
 * @brief Verifies that empty lines or trailing newlines are skipped/handled.
 */
TEST(MapsParserTest, HandlesEmptyLines) {
    std::string_view input = "\n\n1000-2000 r--p 00000000 00:00 0\n\n";
    auto res = dex::MapsParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), 1);
    EXPECT_EQ(res->at(0).start, 0x1000);
}

/**
 * @brief Negative test: Invalid permission string length (3 instead of 4).
 */
TEST(MapsParserTest, InvalidPermissionsLength) {
    std::string_view input = "1000-2000 r-x 00000000 00:00 0";
    auto res = dex::MapsParser::from_string(input);
    
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
}

/**
 * @brief Negative test: Missing address separator.
 */
TEST(MapsParserTest, MissingAddressSeparator) {
    std::string_view input = "1000 2000 r--p 00000000 00:00 0";
    auto res = dex::MapsParser::from_string(input);
    
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
}

/**
 * @brief Negative test: Non-hex address.
 */
TEST(MapsParserTest, NonHexAddress) {
    std::string_view input = "100G-2000 r--p 00000000 00:00 0";
    auto res = dex::MapsParser::from_string(input);
    
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
}
