#include "../src/dex/kv_parser.hpp"
#include <gtest/gtest.h>

/**
 * @brief Test group for basic valid parsing scenarios.
 */
TEST(KVParserTest, BasicSuccess) {
    std::string_view input = "key: 123 0x10 string_val";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    auto& vals = res->at("key");
    ASSERT_EQ(vals.size(), 3);

    // Вместо std::get используйте проверку типа
    EXPECT_TRUE(std::holds_alternative<long>(vals[0])) << "vals[0] is not long";
    EXPECT_TRUE(std::holds_alternative<long>(vals[1])) << "vals[1] is not long";
    EXPECT_TRUE(std::holds_alternative<std::string>(vals[2])) << "vals[2] is not string";

    if (std::holds_alternative<long>(vals[1])) {
        EXPECT_EQ(std::get<long>(vals[1]), 16);
    }
}


/**
 * @brief Verifies that multiple lines are parsed into the map.
 */
TEST(KVParserTest, MultiLineParsing) {
    std::string_view input = "user: admin\nid: 500\ngroups: wheel dev";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), 3);
    EXPECT_EQ(std::get<std::string>(res->at("user")[0]), "admin");
    EXPECT_EQ(std::get<long>(res->at("id")[0]), 500);
    EXPECT_EQ(res->at("groups").size(), 2);
}

/**
 * @brief Verifies numeric priority: digits should be long, mixed should be string.
 */
TEST(KVParserTest, NumericVsStringPriority) {
    std::string_view input = "k1: 1234\nk2: 1234x";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(std::holds_alternative<long>(res->at("k1")[0]));
    EXPECT_TRUE(std::holds_alternative<std::string>(res->at("k2")[0]));
}

/**
 * @brief Tests for empty values after the colon.
 */
TEST(KVParserTest, EmptyValues) {
    std::string_view input = "empty_key:";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->at("empty_key").empty());
}

/**
 * @brief Negative test: Key starting with colon is invalid syntax.
 */
TEST(KVParserTest, KeyStartingWithColon) {
    std::string_view input = ": value";
    auto res = dex::KVParser::from_string(input);
    
    ASSERT_FALSE(res.has_value());
    // Assuming parser_errc is defined in your BaseParser/Error logic
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