//
// timestamp.hpp
// *************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Incomplete. Needs documentation
 */
struct timestamp
{
    int64_t start;
    int64_t end;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, timestamp& m)
{
    if (j.count("start") && !j["start"].is_null())
	{
		if (j["start"].is_number_integer())
			m.start = j["start"];
		else if (j["start"].is_string())
			m.start = std::stoull(j["start"].get<std::string>());
	}
    if (j.count("end") && !j["end"].is_null())
	{
		if (j["end"].is_number_integer())
			m.end = j["end"];
		else if (j["end"].is_string())
			m.end = std::stoull(j["end"].get<std::string>());
	}
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const timestamp& m)
{
    if (m.start != 0)
        j["start"] = m.start;
    if (m.end != 0)
        j["end"] = m.end;
}
/// \endcond

}

}

}
