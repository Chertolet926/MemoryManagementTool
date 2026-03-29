#pragma once
/**
 * @file smaps_entry.hpp
 * @brief Data structure for /proc/[pid]/smaps entries.
 *
 * Defines SmapsEntry structure that combines memory region header
 * with detailed usage statistics.
 */

#include "map_entry.hpp"
#include "kv_registry.hpp"

namespace dex {

    /**
     * @struct SmapsEntry
     * @brief Represents a full block in /proc/[pid]/smaps.
     *
     * Combines the memory region header (from maps) with the
     * detailed usage statistics (from the KV block).
     */
    struct SmapsEntry {
        MapEntry header;  ///< The first line (addresses, perms, path)
        KVRegistry stats; ///< The following block of key-value metrics (Size, Rss, etc.)
    };
}

// Maps SmapsEntry to a Fusion sequence so X3 can populate
// 'header' from the first rule and 'stats' from the second.
BOOST_FUSION_ADAPT_STRUCT(dex::SmapsEntry, header, stats)