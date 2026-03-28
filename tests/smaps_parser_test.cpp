#include <gtest/gtest.h>
#include "dex/smaps_parser.hpp"

using namespace dex;

/**
 * @brief Test suite for composite /proc/[pid]/smaps parsing logic.
 */
class SmapsParserTest : public ::testing::Test {
protected:
    // Helper to simulate a real smaps entry for a library
    const std::string libc_entry = 
        "7f7f3a200000-7f7f3a400000 r-xp 00001000 08:01 123456 /usr/lib/libc.so\n"
        "Size:                2048 kB\n"
        "KernelPageSize:         4 kB\n"
        "MMUPageSize:            4 kB\n"
        "Rss:                 1024 kB\n"
        "Pss:                  512 kB\n"
        "Shared_Clean:         512 kB\n";

    // Helper to simulate an anonymous mapping entry
    const std::string anon_entry =
        "55a2b0c00000-55a2b0c01000 rw-p 00000000 00:00 0 \n"
        "Size:                   4 kB\n"
        "Anonymous:              4 kB\n"
        "LazyFree:               0 kB\n";
};

/**
 * @brief Verifies that a single full smaps block is parsed correctly.
 */
TEST_F(SmapsParserTest, ParsesSingleFullEntry) {
    auto res = SmapsParser::from_string(libc_entry);

    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 1);

    const auto& entry = (*res)[0];
    
    // 1. Verify Header (MapsParser logic)
    EXPECT_EQ(entry.header.start, 0x7f7f3a200000);
    EXPECT_EQ(entry.header.path, "/usr/lib/libc.so");
    EXPECT_TRUE(entry.header.perms.is_x());

    // 2. Verify Stats Registry (KVParser logic)
    EXPECT_EQ(entry.stats.get<long>("Size"), 2048);
    EXPECT_EQ(entry.stats.get<long>("Rss"), 1024);
    EXPECT_EQ(entry.stats.get<long>("Pss"), 512);
    EXPECT_EQ(entry.stats.get<std::string>("Size", 1), "kB");
}

/**
 * @brief Verifies that multiple smaps blocks are parsed in sequence.
 */
TEST_F(SmapsParserTest, ParsesMultipleSequentialEntries) {
    std::string full_input = libc_entry + "\n" + anon_entry;
    auto res = SmapsParser::from_string(full_input);

    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2);

    // First entry
    EXPECT_EQ((*res)[0].header.inode, 123456);
    EXPECT_EQ((*res)[0].stats.get<long>("Rss"), 1024);

    // Second entry
    EXPECT_TRUE((*res)[1].header.path.empty());
    EXPECT_EQ((*res)[1].stats.get<long>("Anonymous"), 4);
}

/**
 * @brief Checks if the parser handles varied value types in smaps (long vs string).
 */
TEST_F(SmapsParserTest, HandlesMixedValueTypesInStats) {
    std::string input = 
        "1000-2000 r--p 00000000 00:00 0 [vvar]\n"
        "VmFlags: rd mr mw me ac sd \n"
        "Size: 4 kB\n";
    
    auto res = SmapsParser::from_string(input);

    ASSERT_TRUE(res.has_value());
    const auto& stats = (*res)[0].stats;

    // VmFlags is a list of strings
    EXPECT_EQ(stats.get<std::string>("VmFlags", 0), "rd");
    EXPECT_EQ(stats.get<std::string>("VmFlags", 5), "sd");

    // Size is long + string
    EXPECT_EQ(stats.get<long>("Size", 0), 4);
    EXPECT_EQ(stats.get<std::string>("Size", 1), "kB");
}

/**
 * @brief Verifies that fallback logic works correctly for smaps metrics.
 */
TEST_F(SmapsParserTest, StatFallbackLogic) {
    auto res = SmapsParser::from_string(anon_entry);
    ASSERT_TRUE(res.has_value());

    // Non-existent metric should return fallback
    EXPECT_EQ((*res)[0].stats.get<long>("NonExistentMetric", 0, -1), -1);
    
    // Asking for a string from a numeric index without a string should return fallback
    EXPECT_EQ((*res)[0].stats.get<std::string>("Size", 0, "N/A"), "N/A");
}

/**
 * @brief Negative test: Corrupted header breaks smaps block.
 */
TEST_F(SmapsParserTest, FailsOnCorruptedHeader) {
    // Missing end address in header
    std::string bad_input = "7f7f3a200000 r-xp 00001000 08:01 123456 /lib/libc.so\nSize: 4 kB";
    auto res = SmapsParser::from_string(bad_input);

    EXPECT_FALSE(res.has_value());
}
