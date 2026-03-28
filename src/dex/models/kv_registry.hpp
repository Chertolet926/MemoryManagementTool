#pragma once

#include <map>
#include <variant>
#include <vector>
#include <string>
#include <string_view>

namespace dex {

    /** @brief A variant type representing either a numeric (long) or a string value. */
    using Value = std::variant<long, std::string>;

    /** @brief Resulting data structure: a map where keys are strings and values are vectors of Value variants. */
    using MapType = std::map<std::string, std::vector<Value>, std::less<>>;

    /** @brief A specialized container for parsed Key-Value pairs with type-safe accessors. */
    struct KVRegistry : MapType {
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
