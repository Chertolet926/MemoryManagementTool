#pragma once
/**
 * @file aliases.hpp
 * @brief Centralized namespace aliases for the MMT codebase.
 *
 * This file provides commonly used namespace aliases to avoid duplication
 * across the codebase. Include this file instead of defining aliases locally.
 *
 * Usage:
 * @code
 *   #include "common/aliases.hpp"
 * @endcode
 *
 * Provided aliases:
 *   - fs       → std::filesystem
 *   - dex::x3  → boost::spirit::x3 (accessible as 'x3' inside dex namespace)
 */

#include <filesystem>
#include <boost/spirit/home/x3.hpp>

/**
 * @namespace fs
 * @brief Alias for std::filesystem to reduce verbosity.
 */
namespace fs = std::filesystem;

/**
 * @namespace dex::x3
 * @brief Alias for boost::spirit::x3 within the dex namespace.
 *
 * This allows using x3 parsers with shorter syntax inside the dex namespace.
 */
namespace dex {
    namespace x3 = boost::spirit::x3;
}
