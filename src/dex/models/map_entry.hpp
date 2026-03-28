#pragma once

#include <boost/fusion/include/adapt_struct.hpp>
#include <cstdint>
#include <string>

namespace dex {
    
    /**
     * @struct Perms
     * @brief A compact bitfield representing memory segment permissions.
     * 
     * Uses bitfields to compress four boolean flags into a single byte of memory.
     * Maps directly to the 'rwxp' or 'rwxs' string found in /proc/maps.
     */
    struct Perms {
        bool readable   : 1; /// 'r' - Read permission
        bool writable   : 1; /// 'w' - Write permission
        bool executable : 1; /// 'x' - Execute permission
        bool shared     : 1; /// 's' - Shared memory (vs 'p' - private)

        // --- Helper Methods ---
        bool is_r() const { return readable; }
        bool is_w() const { return writable; }
        bool is_x() const { return executable; }
        bool is_private() const { return !shared; }
    };

    /** 
     * @struct MapEntry
     * @brief Represents a single memory-mapped region from a /proc/[pid]/maps file.
     * 
     * This structure serves as the AST (Abstract Syntax Tree) for the MapsParser.
     */
    struct MapEntry {
        uint64_t start;   /// Starting virtual address of the mapping
        uint64_t end;     /// Ending virtual address (exclusive)
        Perms perms;      /// Parsed permissions bitfield
        uint64_t offset;  /// Offset into the file or shared memory object
        std::string dev;  /// Device identifier (major:minor)
        uint64_t inode;   /// Inode number on the device
        std::string path; /// Path to the mapped file, or special tags ([stack], [heap], etc.)

        /** @brief Calculates the total size of the memory region in bytes. */
        uint64_t size() const noexcept { return end - start; }
    };
}

// Maps the MapEntry struct fields to a Fusion sequence.
BOOST_FUSION_ADAPT_STRUCT(dex::MapEntry, 
    start, end, perms, offset, dev, inode, path
)