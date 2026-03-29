#pragma once
/**
 * @file kv_parser.hpp
 * @brief Parser for key-value pair registries.
 *
 * Provides parsing for key-value format files where each line contains
 * a key followed by colon and one or more values.
 */

#include <boost/fusion/adapted/std_pair.hpp>
#include "models/kv_registry.hpp"
#include "base_parser.hpp"
#include "common/aliases.hpp"
#include <variant>
#include <vector>
#include <string>
#include <map>

namespace dex {

    /**
     * @namespace kv_rules
     * @brief Grammar rules for the Key-Value (KV) format.
     */
    namespace kv_rules {

        // Rule declarations with explicit attribute types.
        // These guide X3 when mapping parsed text to C++ structures.
        inline x3::rule<struct val_id, Value> const val = "val";
        inline x3::rule<struct str_id, std::string> const str = "str";
        inline x3::rule<struct line_id, std::pair<std::string, std::vector<Value>>> const line = "line";

        // --- Definitions ---

        /**
         * @brief End-of-Word (EOW) predicate.
         *
         * Matches when not followed by a printable character.
         */
        auto const eow = !x3::graph;

        /**
         * @brief String parser: any printable character sequence excluding whitespace.
         */
        auto const str_def   = x3::lexeme[+(x3::graph - x3::char_(" \t\n\r"))];

        /**
         * @brief Value parser: supports hexadecimal (0x...), decimals, or strings.
         */
        auto const val_def   = (x3::lexeme["0x" >> x3::hex >> eow]) | x3::lexeme[x3::long_ >> eow] | str;

        /**
         * @brief Key parser: sequence of printable characters up to the colon separator.
         */
        auto const key       = x3::lexeme[+(x3::graph - ':')];

        /**
         * @brief Line parser: single line consisting of "key: value1 value2 ..."
         *
         * @note *val consumes values separated by implied blanks when used with a skipper.
         */
        auto const line_def  = key >> ':' >> *val;

        /**
         * @brief Top-level parser: matches lines separated by newlines, allowing trailing EOLs.
         */
        auto const config    = (line % x3::eol) >> *x3::eol;

        // Binds rule declarations to their actual logic definitions.
        BOOST_SPIRIT_DEFINE(str, val, line);
    }

    /**
     * @class KVParser
     * @brief Specialized parser for key-value pair registries.
     *
     * Inherits from BaseParser to utilize common parsing infrastructure.
     * Expected format: "key: val1 val2"
     */
    struct KVParser : BaseParser<KVParser, KVRegistry> {
        /**
         * @brief Returns the human-readable identifier for this parser.
         * @return "KVParser" string literal.
         */
        static constexpr const char* name() { return "KVParser"; }

        /**
         * @brief Returns the entry point for the X3 grammar.
         * @return Grammar rule for KV parsing.
         */
        static auto grammar() { return kv_rules::config; }
    };
} // namespace dex
