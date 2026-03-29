#pragma once
/**
 * @file map_entry.hpp
 * @brief Data structures for memory mapping entries.
 *
 * Defines Perms and MapEntry structures used by MapsParser.
 */

#include <boost/fusion/include/adapt_struct.hpp>
#include <cstdint>
#include <string>
#include <optional>

namespace dex {

    /**
     * @struct Perms
     * @brief Compact bitfield representing memory segment permissions.
     *
     * Uses bitfields to compress four boolean flags into a single byte of memory.
     * Maps directly to the 'rwxp' or 'rwxs' string found in /proc/maps.
     */
    struct Perms {
        bool readable   : 1;  ///< 'r' - Read permission
        bool writable   : 1;  ///< 'w' - Write permission
        bool executable : 1;  ///< 'x' - Execute permission
        bool shared     : 1;  ///< 's' - Shared memory ('p' = private)

        // --- Helper Methods ---

        /**
         * @brief Check if memory is readable.
         * @return true if readable flag is set.
         */
        bool is_r() const { return readable; }

        /**
         * @brief Check if memory is writable.
         * @return true if writable flag is set.
         */
        bool is_w() const { return writable; }

        /**
         * @brief Check if memory is executable.
         * @return true if executable flag is set.
         */
        bool is_x() const { return executable; }

        /**
         * @brief Check if memory is private (not shared).
         * @return true if private (shared flag is false).
         */
        bool is_private() const { return !shared; }
    };

    /**
     * @struct MapEntry
     * @brief Represents a single memory-mapped region from /proc/[pid]/maps.
     *
     * This structure serves as the AST (Abstract Syntax Tree) for the MapsParser.
     *
     * @note Invariant: end >= start (enforced by parser, validated in size()).
     */
    struct MapEntry {
        uint64_t start;   ///< Starting virtual address of the mapping
        uint64_t end;     ///< Ending virtual address (exclusive)
        Perms perms;      ///< Parsed permissions bitfield
        uint64_t offset;  ///< Offset into the file or shared memory object
        std::string dev;  ///< Device identifier (major:minor)
        uint64_t inode;   ///< Inode number on the device
        std::string path; ///< Path to the mapped file, or special tags ([stack], [heap], etc.)

        /**
         * @brief Calculates the total size of the memory region in bytes.
         * @return Size in bytes, or std::nullopt if end < start (invalid mapping).
         *
         * @note This function validates the invariant in ALL builds (Debug and Release).
         *       The parser should guarantee end >= start, but this provides defense in depth.
         */
        [[nodiscard]] std::optional<uint64_t> size() const noexcept {
            if (end < start) return std::nullopt;
            return end - start;
        }

        /**
         * @brief Calculates the size, returning 0 for invalid mappings.
         * @return Size in bytes (0 if invalid).
         *
         * @note Use this when you need a simple value and can tolerate 0 for invalid data.
         */
        [[nodiscard]] uint64_t size_or_zero() const noexcept {
            return (end >= start) ? (end - start) : 0;
        }
    };
}

// Maps the MapEntry struct fields to a Fusion sequence.
BOOST_FUSION_ADAPT_STRUCT(dex::MapEntry, 
    start, end, perms, offset, dev, inode, path
)