#pragma once

#include "base_parser.hpp"
#include "models/smaps_entry.hpp"
#include "maps_parser.hpp"
#include "kv_parser.hpp"

namespace dex {

    // Composite grammar that reuses existing Maps and KV rules.
    namespace smaps_rules {
        namespace x3 = boost::spirit::x3;

        // Forward declaration of rules with explicit attributes.
        // This prevents type-inference recursion and ensures correct 
        // propagation of MapEntry and KVRegistry data structures.
        x3::rule<struct stats_block_id, KVRegistry> const stats_block = "stats_block";
        x3::rule<struct smaps_entry_id, SmapsEntry> const smaps_entry = "smaps_entry";

        // Statistics block definition.
        // Consumes multiple Key-Value lines. Each line must end with a newline (eol).
        // The parser stops here when it encounters a line that does not contain a colon
        // (usually the start of the next memory segment header).
        auto const stats_block_def = +(dex::kv_rules::line >> x3::eol);

    
        // Single SMAPS entry definition.
        auto const smaps_entry_def = dex::maps_rules::line >> x3::eol >> stats_block;

        // Top-level SMAPS configuration grammar.
        auto const smaps_config = +(smaps_entry | x3::omit[x3::eol]);

        // Binds the rule declarations to their specific definitions
        BOOST_SPIRIT_DEFINE(stats_block, smaps_entry);
    }

    /**
     * @class SmapsParser
     * @brief Parser for Linux /proc/[pid]/smaps.
     * 
     * This parser effectively nests the KVParser inside the MapsParser structure.
     * Resulting data type: std::vector<SmapsEntry>.
     */
    struct SmapsParser : BaseParser<SmapsParser, std::vector<SmapsEntry>> {
        // Diagnostic name for the parser.
        static constexpr const char* name() { return "SmapsParser"; }
        
        // Entry point for the composite smaps grammar.
        static auto grammar() { return smaps_rules::smaps_config; }
    };

} // namespace dex