#pragma once

#include <boost/fusion/adapted/std_pair.hpp>
#include "base_parser.hpp"
#include <map>
#include <variant>
#include <vector>
#include <string>

namespace dex {

    // A variant type representing either a numeric (long) or a string value.
    using Value = std::variant<long, std::string>;
    // Resulting data structure: a map where keys are strings and values are vectors of Value variants.
    using MapType = std::map<std::string, std::vector<Value>, std::less<>>;

    // Internal grammar rules for the Key-Value (KV) format.
    namespace kv_rules {
        namespace x3 = boost::spirit::x3;

        // Rule declarations with explicit attribute types.
        // These are required to guide X3 when mapping parsed text to C++ structures (variant/pair/vector).
        inline x3::rule<struct val_id, Value> const val = "val";
        inline x3::rule<struct str_id, std::string> const str = "str";
        inline x3::rule<struct line_id, std::pair<std::string, std::vector<Value>>> const line = "line";

        // --- Definitions ---

        // End-of-Word (EOW) predicate.
        auto const eow = !x3::graph; 

        // str_def: Any printable character sequence excluding whitespace.
        auto const str_def   = x3::lexeme[+(x3::graph - x3::char_(" \t\n\r"))];
        
        // val_def: Supports hexadecimal (0x...), standard decimals, or fallback to strings.
        auto const val_def   = (x3::lexeme["0x" >> x3::hex >> eow]) | x3::lexeme[x3::long_ >> eow] | str; 
        
        // key: A sequence of printable characters up to the colon separator.
        auto const key       = x3::lexeme[+(x3::graph - ':')];
    
        // line_def: A single line consisting of "key: value1 value2 ..."
        // Note: *val will consume values separated by implied blanks if used with a skipper.
        auto const line_def  = key >> ':' >> *val;
        
        // config: The top-level parser. Matches lines separated by newlines, allowing trailing EOLs.
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
    struct KVParser : BaseParser<KVParser, MapType> {
        // Returns the human-readable identifier for this parser.
        static constexpr const char* name() { return "KVParser"; }
        
        // Returns the entry point for the X3 grammar.
        static auto grammar() { return kv_rules::config; }
    };
} // namespace dex
