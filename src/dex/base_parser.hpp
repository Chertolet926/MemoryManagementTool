#pragma once
/**
 * @file base_parser.hpp
 * @brief Base class for Spirit X3 parsers using CRTP.
 *
 * Provides a reusable foundation for building parsers with common
 * functionality like stream, string, and file parsing.
 */

#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/system/detail/errc.hpp>
#include "common/errors.hpp"
#include "common/aliases.hpp"
#include <expected>
#include <fstream>
#include <string>

namespace dex {

    /**
     * @class BaseParser
     * @brief Base class for Spirit X3 parsers using CRTP (Curiously Recurring Template Pattern).
     *
     * @tparam Derived The actual parser class (e.g., CpuParser).
     * @tparam T The data structure to be populated by the parser.
     *
     * This class provides common parsing infrastructure:
     * - from_stream(): Parse from std::istream
     * - from_string(): Parse from string_view
     * - from_file(): Parse from filesystem path
     *
     * @note Derived classes must provide:
     *   - static constexpr const char* name() - Parser diagnostic name
     *   - static auto grammar() - Spirit X3 grammar definition
     */
    template <typename Derived, typename T> class BaseParser {
    private:
        /**
         * @brief Internal helper to wrap error data into a std::unexpected result.
         * @param ec Error code (system or parser specific).
         * @param off Byte offset where the error occurred (default 0).
         * @return Failure result containing parser error details.
         */
        static auto error(boost::system::error_code ec, size_t off = 0) {
            return std::unexpected(parser_error{ec, Derived::name(), off});
        }

    public:
        /// Type alias for the expected result: either the data T or a parser_error
        using result_t = std::expected<T, parser_error>;

        /**
         * @brief Core parsing logic that processes data from a generic input stream.
         * @param is Input stream to read from.
         * @return Result containing parsed data or error description.
         */
        [[nodiscard]] static result_t from_stream(std::istream &is) {
      // Buffer the entire stream content into a string for contiguous memory access
      std::string content((std::istreambuf_iterator<char>(is)), {});
      if (is.bad()) return error(make_error_code(boost::system::errc::io_error));

      T result; auto it = content.begin();

      // Execute the Spirit X3 grammar defined in the Derived class
      if (x3::phrase_parse(it, content.end(), Derived::grammar(), x3::blank, result))
        // Ensure the entire input was consumed (no trailing garbage)  
        if (it == content.end()) return result;
  
      // Report syntax error with the current iterator position
      return error(make_error_code(parser_errc::invalid_syntax), 
        static_cast<size_t>(std::distance(content.begin(), it)));
    }

        /**
         * @brief Convenience method to parse data directly from a string or string_view.
         * @param s String-like data source.
         * @return Parsing result.
         */
        [[nodiscard]] static result_t from_string(std::string_view s) {
      // Use ibufferstream to wrap existing memory without copying data
      boost::interprocess::ibufferstream iss(s.data(), s.size());
      return from_stream(iss);
    }

        /**
         * @brief Method to parse data directly from a file on disk.
         * @param path Filesystem path to the file.
         * @return Parsing result or system error (e.g., File Not Found).
         */
        [[nodiscard]] static result_t from_file(const std::string& path) {
      // Attempt to open the file in binary mode
      if (std::ifstream ifs{path, std::ios::binary}) return from_stream(ifs);
      return error({errno, boost::system::system_category()});
    }
    };

} // namespace dex