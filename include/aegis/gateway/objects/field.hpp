//
// field.hpp
// *********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
struct field
{
    field(std::string n, std::string v) : _name(n), _value(v) {}
    field(std::string n, std::string v, bool il) : _name(n), _value(v), _is_inline(il) {}
    field() = default;

    /// Sets the name of the field
    /**
     * @param str Url to set
     */
    field & name(const std::string & str)
    {
        _name = str;
        return *this;
    }

    /// Sets the value of the field
    /**
     * @param str Value to set
     */
    field & value(const std::string & str)
    {
        _value = str;
        return *this;
    }

    /// Sets whether the field is inline
    /**
     * @param b Is inline or not
     */
    field & is_inline(const bool b)
    {
        _is_inline = b;
        return *this;
    }

    /// Get the name of the field
    std::string & name() noexcept
    {
        return _name;
    }

    /// Get the value of the field
    std::string & value() noexcept
    {
        return _value;
    }

    /// Get whether the field is inline
    bool & is_inline() noexcept
    {
        return _is_inline;
    }

private:
    std::string _name; /**<\ Name of the field */
    std::string _value; /**<\ Value of the field */
    bool _is_inline = false; /**<\ Whether the field is inline or not */
    friend void from_json(const nlohmann::json& j, field& m);
    friend void to_json(nlohmann::json& j, const field& m);
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, field& m)
{
    if (j.count("name"))
        m._name = j["name"].get<std::string>();
    if (j.count("value"))
        m._value = j["value"].get<std::string>();
    if (j.count("inline"))
        m._is_inline = j["inline"];
}

inline void to_json(nlohmann::json& j, const field& m)
{
    j["name"] = m._name;
    j["value"] = m._value;
    j["inline"] = m._is_inline;
}
/// \endcond

}

}

}
