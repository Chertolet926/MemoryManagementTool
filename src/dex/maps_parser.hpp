#pragma once

#include <boost/fusion/include/adapt_struct.hpp>
#include "models/map_entry.hpp"
#include "base_parser.hpp"

namespace dex {

    // Internal Spirit X3 grammar rules for parsing Linux /proc/[pid]/maps files.
    namespace maps_rules {
        // Hexadecimal parser for 64-bit addresses (e.g., 55a2b0c00000).
        x3::uint_parser<uint64_t, 16> const hex64;

        // --- Rule Declarations with Attributes ---
        x3::rule<struct perms_id, Perms> const perms = "perms";
        x3::rule<struct dev_id, std::string> const dev = "dev";
        x3::rule<struct path_id, std::string> const path = "path";
        x3::rule<struct line_id, MapEntry> const line = "line";

        // --- Definitions ---

        // Parses exactly 4 characters and converts them into a Perms object.
        auto const perms_def = x3::lexeme[x3::repeat(4)[x3::char_("rwxps-")]][([](auto& ctx) {
            auto const& attr = x3::_attr(ctx);
            Perms p{
                attr[0] == 'r',
                attr[1] == 'w',
                attr[2] == 'x',
                attr[3] == 's'  // 's' = shared, 'p'/' - ' = private
            };
            x3::_val(ctx) = p;
        })];

        // Device: major:minor numbers in hex format (e.g., "08:01").
        auto const dev_def = x3::lexeme[+x3::graph];

        // Optional path/mapping name (e.g., "/usr/lib/libc.so", "[stack]", or empty).
        auto const path_def = x3::lexeme[*x3::graph];

        // Single line format: start-end perms offset dev inode path
        auto const line_def = hex64 >> x3::lit('-') >> hex64 >> perms
            >> hex64 >> dev >> x3::long_ >> path;

        // Top-level grammar: zero or more map entries, 
        auto const config = *(line | x3::omit[+x3::eol]);

        // Link declarations to definitions for X3 processing.
        BOOST_SPIRIT_DEFINE(perms, dev, path, line);
    }

    /**
     * @class MapsParser
     * @brief Concrete implementation of BaseParser for memory mapping files.
     * 
     * Inherits from BaseParser to gain from_file/from_string functionality.
     * Resulting data type: std::vector<MapEntry>.
     */
    struct MapsParser : BaseParser<MapsParser, std::vector<MapEntry>> {
        // Returns the human-readable identifier for this parser.
        static constexpr const char* name() { return "MapsParser"; }
        
        // Returns the entry point for the X3 grammar.
        static auto grammar() { return maps_rules::config; }
    };
}