#pragma once
/**
 * @file kv_registry.hpp
 * @brief Data structures for key-value registry.
 *
 * Defines Value variant and KVRegistry container for parsed key-value pairs.
 */

#include <map>
#include <variant>
#include <vector>
#include <string>
#include <string_view>

namespace dex {

    /**
     * @using Value
     * @brief Variant type representing either a numeric (long) or string value.
     */
    using Value = std::variant<long, std::string>;

    /**
     * @using MapType
     * @brief Map where keys are strings and values are vectors of Value variants.
     *
     * Uses transparent comparator (std::less<>) for heterogeneous lookups.
     */
    using MapType = std::map<std::string, std::vector<Value>, std::less<>>;

    /**
     * @struct KVRegistry
     * @brief Specialized container for parsed Key-Value pairs with type-safe accessors.
     *
     * Inherits from MapType and provides convenient typed access to values.
     */
    struct KVRegistry : MapType {
        /**
         * @brief Retrieves a typed value by key.
         *
         * @tparam T Type to retrieve (long or std::string).
         * @param key Key to look up.
         * @param idx Index in the value vector (default 0).
         * @param fallback Default value if key not found or type mismatch.
         * @return The typed value or fallback on failure.
         */
        template <typename T>
        T get(std::string_view key, size_t idx = 0, T fallback = T{}) const noexcept {
            auto it = this->find(key);
            if (it != this->end() && idx < it->second.size()) {
                if (auto* p = std::get_if<T>(&it->second[idx]))
                    return *p;
            }
            return fallback;
        }
    };

} // namespace dex
