//
// permission_overwrite.hpp
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
#include "../structs.hpp"
#include <json.hpp>
#include <string>
#include <vector>



namespace aegiscpp
{

class member;
class channel;

/**\todo Needs documentation
*/
struct permission_overwrite
{
    snowflake id; /**<\todo Needs documentation */
    //either "role" or "member"
    overwrite_type type; /**<\todo Needs documentation */
    int64_t allow = 0; /**<\todo Needs documentation */
    int64_t deny = 0; /**<\todo Needs documentation */
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, permission_overwrite& m)
{
    m.id = j["id"];
    if (j.count("type"))
        m.type = (j["type"] == "role") ? (overwrite_type::Role) : (overwrite_type::User);
    m.allow = j["allow"];
    m.deny = j["deny"];
}

/**\todo Needs documentation
*/
inline void to_json(nlohmann::json& j, const permission_overwrite& m)
{
    j["id"] = m.id;
    j["type"] = m.type;
    j["allow"] = m.allow;
    j["deny"] = m.deny;
}

}
