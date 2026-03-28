#include "../src/dex/kv_parser.hpp"
#include <gtest/gtest.h>

/**
 * @brief Basic success scenario using the new get<T>() helper.
 */
TEST(KVParserTest, BasicSuccess) {
    std::string_view input = "key: 123 0x10 string_val";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    
    // Test successful retrieval with correct types
    EXPECT_EQ(res->get<long>("key", 0), 123);
    EXPECT_EQ(res->get<long>("key", 1), 16);
    EXPECT_EQ(res->get<std::string>("key", 2), "string_val");

    // Test fallback for non-existent index or wrong type
    EXPECT_EQ(res->get<long>("key", 99, -1), -1);
    EXPECT_EQ(res->get<std::string>("key", 0, "fallback"), "fallback"); // Index 0 is long, not string
}

/**
 * @brief Multi-line parsing with structured access.
 */
TEST(KVParserTest, MultiLineParsing) {
    std::string_view input = "user: admin\nid: 500\ngroups: wheel dev";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), 3);
    
    EXPECT_EQ(res->get<std::string>("user"), "admin");
    EXPECT_EQ(res->get<long>("id"), 500);
    EXPECT_EQ(res->get<std::string>("groups", 0), "wheel");
    EXPECT_EQ(res->get<std::string>("groups", 1), "dev");
}

/**
 * @brief Verifies numeric priority and mixed types.
 */
TEST(KVParserTest, NumericVsStringPriority) {
    std::string_view input = "k1: 1234\nk2: 1234x";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    
    // k1 should be long
    EXPECT_EQ(res->get<long>("k1"), 1234);
    // k2 should be string because of the 'x'
    EXPECT_EQ(res->get<std::string>("k2"), "1234x");
    
    // Cross-check: asking for long on k2 should return fallback
    EXPECT_EQ(res->get<long>("k2", 0, -7), -7);
}

/**
 * @brief Tests for empty values after the colon.
 */
TEST(KVParserTest, EmptyValues) {
    std::string_view input = "empty_key:";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    // find() still works because KVRegistry inherits from MapType
    auto it = res->find("empty_key");
    ASSERT_NE(it, res->end());
    EXPECT_TRUE(it->second.empty());
    
    // get() should return fallback for empty values
    EXPECT_EQ(res->get<long>("empty_key", 0, 999), 999);
}

/**
 * @brief Negative test: Key starting with colon is invalid syntax.
 */
TEST(KVParserTest, KeyStartingWithColon) {
    std::string_view input = ": value";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
}

/**
 * @brief Negative test: Input without any colon should fail.
 */
TEST(KVParserTest, MissingColon) {
    std::string_view input = "key_no_colon_value";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
}

/**
 * @brief Verifies fallback logic for non-existent keys.
 */
TEST(KVParserTest, FallbackLogic) {
    auto res = dex::KVParser::from_string("a: 1");
    ASSERT_TRUE(res.has_value());
    
    EXPECT_EQ(res->get<long>("missing_key", 0, 404), 404);
    EXPECT_EQ(res->get<std::string>("a", 10, "out_of_bounds"), "out_of_bounds");
}
