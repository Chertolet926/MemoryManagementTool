#pragma once
/**
 * @file statm_parser.hpp
 * @brief Parser for /proc/[pid]/statm files.
 *
 * Provides parsing for process memory statistics including size, resident,
 * shared, text, library, data, and dirty pages.
 */

#include "base_parser.hpp"
#include "models/statm_entry.hpp"
#include "common/aliases.hpp"

namespace dex {
    /**
     * @namespace statm_rules
     * @brief Spirit X3 grammar rules for statm parsing.
     */
    namespace statm_rules {

        /**
         * @brief Statm grammar definition.
         *
         * Matches exactly seven 64-bit unsigned integers separated by spaces.
         * Format: size resident shared text lib data dt
         */
        auto const config =
            x3::uint64 >> x3::uint64 >> x3::uint64 >>
            x3::uint64 >> x3::uint64 >> x3::uint64 >> x3::uint64;
    }

    /**
     * @class StatmParser
     * @brief Specialized parser for /proc/[pid]/statm files.
     *
     * Parses memory statistics into a StatmEntry structure.
     * All values are represented in pages.
     */
    struct StatmParser : BaseParser<StatmParser, StatmEntry> {
        /**
         * @brief Returns the diagnostic name for this parser.
         * @return "StatmParser" string literal.
         */
        static constexpr const char* name() { return "StatmParser"; }

        /**
         * @brief Returns the entry point for the Spirit X3 grammar.
         * @return Grammar rule for statm parsing.
         */
        static auto grammar() { return statm_rules::config; }
    };
}
