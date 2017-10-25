//
// emoji.hpp
// aegis.cpp
//
// Copyright (c) 2017 Sara W (sara at xandium dot net)
//
// This file is part of aegis.cpp .
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once


#include "../config.hpp"
#include "../snowflake.hpp"
#include <string>
#include <vector>



namespace aegiscpp
{


struct emoji
{
    snowflake emoji_id;
    std::string name;
    std::vector<snowflake> roles;
    snowflake user;
    bool require_colons = false;
    bool managed = false;
};

void from_json(const nlohmann::json& j, emoji& m)
{
    m.emoji_id = j["id"];
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"];
    if (j.count("user") && !j["user"].is_null())
        m.user = j["user"]["id"];
    if (j.count("require_colons") && !j["require_colons"].is_null())
        m.require_colons = j["require_colons"];
    if (j.count("managed") && !j["managed"].is_null())
        m.managed = j["managed"];
    if (j.count("roles") && !j["roles"].is_null())
        for (auto i : j["roles"])
            m.roles.push_back(i);
}
void to_json(nlohmann::json& j, const emoji& m)
{
    j["id"] = m.emoji_id;
    j["name"] = m.name;
    j["user"] = m.user;
    j["require_colons"] = m.require_colons;
    j["managed"] = m.managed;
    for (auto i : m.roles)
        j["roles"].push_back(i);
}

}

