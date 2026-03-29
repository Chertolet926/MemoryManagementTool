#pragma once
/**
 * @file statm_entry.hpp
 * @brief Data structure for /proc/[pid]/statm memory statistics.
 *
 * Defines StatmEntry structure used by StatmParser.
 */

#include <cstdint>
#include <boost/fusion/include/adapt_struct.hpp>

namespace dex {

    /**
     * @struct StatmEntry
     * @brief Represents memory usage statistics from /proc/[pid]/statm.
     *
     * All values are represented in pages.
     */
    struct StatmEntry {
        uint64_t size;     ///< Total program size (pages)
        uint64_t resident; ///< Resident set size (pages)
        uint64_t shared;   ///< Shared pages (i.e., backed by a file)
        uint64_t text;     ///< Text (code) pages
        uint64_t lib;      ///< Library (unused in Linux 2.6+)
        uint64_t data;     ///< Data + stack pages
        uint64_t dt;       ///< Dirty pages (unused in Linux 2.6+)
    };
}

// Map the struct for Spirit X3
BOOST_FUSION_ADAPT_STRUCT(dex::StatmEntry,
    size, resident, shared, text, lib, data, dt
)
