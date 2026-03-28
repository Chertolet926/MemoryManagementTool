#pragma once
#include "base_parser.hpp"
#include "models/statm_entry.hpp"

namespace dex {
    namespace statm_rules {
        namespace x3 = boost::spirit::x3;

        // Statm grammar definition.
        // Matches exactly seven 64-bit unsigned integers separated by spaces.
        auto const config = 
            x3::uint64 >> x3::uint64 >> x3::uint64 >> 
            x3::uint64 >> x3::uint64 >> x3::uint64 >> x3::uint64;
    }

    /**
     * @class StatmParser
     * @brief Specialized parser for /proc/[pid]/statm files.
     */
    struct StatmParser : BaseParser<StatmParser, StatmEntry> {
        // Diagnostic name for the parser.
        static constexpr const char* name() { return "StatmParser"; }
        
        // Entry point for the composite smaps grammar.
        static auto grammar() { return statm_rules::config; }
    };
}
