//
// snowflake.cpp
// *************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, snowflake& s)
{
    if (j.is_string())
        s = std::stoll(j.get<std::string>());
    else if (j.is_number())
        s = j.get<int64_t>();
}

AEGIS_DECL void to_json(nlohmann::json& j, const snowflake& s)
{
    j = nlohmann::json{ static_cast<int64_t>(s) };
}
/// \endcond

}
