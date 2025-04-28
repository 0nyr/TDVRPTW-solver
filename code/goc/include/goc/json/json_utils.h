//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#ifndef GOC_JSON_JSON_UTILS_H
#define GOC_JSON_JSON_UTILS_H

#include <string>
#include <magic_enum/magic_enum.hpp>

#include "goc/lib/json.hpp"

namespace goc
{
// Returns: if the object has the specified key defined.
bool has_key(const nlohmann::json& object, const std::string& key);

// Returns: The object value for the key if it is defined, otherwise returns def.
const nlohmann::json& value_or_default(const nlohmann::json& object, const std::string& key, const nlohmann::json& def);

/**
 * Get an enum value from a JSON object, or return a default value 
 * if the key is not present or the value is invalid.
 */
template<typename T>
T enum_value_or_default(const nlohmann::json& object, const std::string& key, const T& def)
{
    if (!has_key(object, key))
        return def;

    if constexpr (std::is_enum_v<T>)
    {
        // Enum case
        auto maybe_enum = magic_enum::enum_cast<T>(object[key].get<std::string>());
        if (maybe_enum.has_value())
            return maybe_enum.value();
        else
            return def;
    }
    else
    {
        // Normal case
        return object[key].get<T>();
    }
}

} // namespace goc

// Add implementations for the to_json of common objects.
namespace std
{
// to_json implementation of a generic vector of elements.
template<typename T>
void to_json(nlohmann::json& j, const std::vector<T>& v)
{
	j = vector<nlohmann::json>();
	for (auto& e: v) j.push_back(e);
}
} // namespace std

#endif //GOC_JSON_JSON_UTILS_H
