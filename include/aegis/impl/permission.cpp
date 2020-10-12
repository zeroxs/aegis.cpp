//
// permission.cpp
// **************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//
// Revision by LSW
//

#include "aegis/config.hpp"
#include "aegis/permission.hpp"
#include <nlohmann/json.hpp>
#include <stdint.h>

namespace aegis
{

    /// \cond TEMPLATES
    AEGIS_DECL void from_json(const nlohmann::json& j, permission& s)
    {
        if (j.is_string())
            s = std::stoll(j.get<std::string>());
        else if (j.is_number())
            s = j.get<int64_t>();
    }

    AEGIS_DECL void to_json(nlohmann::json& j, const permission& s)
    {
        j = nlohmann::json{ static_cast<int64_t>(s) };
    }
    /// \endcond

}
