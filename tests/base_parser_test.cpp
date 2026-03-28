#include "../src/dex/base_parser.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

struct DummyParser : public dex::BaseParser<DummyParser, int> {
    static constexpr std::string_view name() { return "DUMMY"; }
    static auto grammar() { return dex::x3::int_; }
};

/**
 * @test Test successful parsing from string
 */
TEST(BaseParserTest, Success) {
    auto res = DummyParser::from_string("100");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(*res, 100);
}

/**
 * @test Test syntax error and offset reporting
 */
TEST(BaseParserTest, SyntaxErrorOffset) {
    auto res = DummyParser::from_string("abc");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
    EXPECT_EQ(res.error().offset, 0); // Fails at the very beginning
}

/**
 * @test Test trailing garbage (important logic check)
 */
TEST(BaseParserTest, TrailingGarbage) {
    // Parser reads '100', but ' extra' remains
    auto res = DummyParser::from_string("100 extra"); 
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
    EXPECT_EQ(res.error().offset, 4u); // Error starts at space after '100'
}

/**
 * @test Test OS error: File Not Found
 */
TEST(BaseParserTest, FileNotFound) {
    auto res = DummyParser::from_file("/non/existent/path/9999");
    ASSERT_FALSE(res.has_value());
    // Should be a system category error (ENOENT)
    EXPECT_EQ(res.error().ec.category(), boost::system::system_category());
    EXPECT_EQ(res.error().ec.value(), ENOENT);
}
 
/**
 * @test Verifies that the parser correctly handles OS-level permission errors.
 */
TEST(BaseParserTest, FromFileAccessDenied) {
    const std::string path = "restricted.txt";
    namespace fs = std::filesystem;

    { std::ofstream{path} << "42"; }

    fs::permissions(path, fs::perms::none);

    auto res = DummyParser::from_file(path);
    
    fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write);
    fs::remove(path);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec.value(), EACCES);
    EXPECT_EQ(res.error().parser_name, "DUMMY");
}

/**
 * @test Verifies that the parser successfully reads and processes a valid file.
 */
TEST(BaseParserTest, FromFileSuccess) {
    const std::string path = "temp_dummy.txt";
    
    { std::ofstream{path, std::ios::binary} << "2048"; }

    auto res = DummyParser::from_file(path);

    std::filesystem::remove(path);

    ASSERT_TRUE(res.has_value()) << "Failed to parse: " << res.error().ec.message();
    EXPECT_EQ(*res, 2048);
}

/**
 * @test Test empty input handling
 */
TEST(BaseParserTest, EmptyInput) {
    auto res = DummyParser::from_string("");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().ec, dex::parser_errc::invalid_syntax);
}