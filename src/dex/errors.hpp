#pragma once

#include <boost/system/detail/error_code.hpp>
#include <boost/system/error_code.hpp>
#include <string_view>

namespace dex {
  /// Internal parser logic error codes
  enum class parser_errc { success = 0, invalid_syntax, incomplete_input };

  /// Definition of the error category for integration with boost::system
  struct parser_category : boost::system::error_category {
    
    /**
     * @brief Returns the unique identifier for this error category.
     * This allows the system to distinguish between OS errors and DEX parser errors.
     * @return A string literal "dex_parser".
     */
    const char *name() const noexcept override { return "dex_parser"; };

    /**
     * @brief Converts a numeric error value into a human-readable string.
     * @param ev The integer representation of a parser_errc.
     * @return A descriptive error message.
     */
    std::string message(int ev) const override {
      switch (static_cast<parser_errc>(ev)) {
      case parser_errc::invalid_syntax:
        return "Invalid syntax";
      case parser_errc::incomplete_input:
        return "Incomplete input";
      default:
        return "Unknown parser error";
      }
    }
  };

  /**
   * @brief Returns a reference to the static dex error category instance.
   * Required for creating boost::system::error_code objects.
   * @return A reference to the singleton parser_category.
   */
  inline const boost::system::error_category &get_parser_category() {
    static parser_category instance;
    return instance;
  }

  /**
   * @brief Factory function to create a boost::system::error_code from a parser_errc.
   * @param e The specific parser error code.
   * @return A complete error_code object containing the value and the dex category.
   */
  inline boost::system::error_code make_error_code(parser_errc e) {
    return {static_cast<int>(e), get_parser_category()};
  }

  /// Main error reporting structure
  struct parser_error {
    boost::system::error_code ec; /// The error code (either OS system error or dex_parser error)
    std::string_view parser_name; // The name of the specific parser where the failure occurred (e.g., "CPU_PARSER")
    size_t offset = 0; /// Byte offset in the input stream where parsing failed
  };
} // namespace dex

/// Registration with Boost.System to enable automatic error code conversions
namespace boost::system {
  template <> struct is_error_code_enum<dex::parser_errc> : std::true_type {};
} // namespace boost::system