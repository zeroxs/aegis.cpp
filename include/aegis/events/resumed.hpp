//
// resumed.hpp
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
#include "../objects/message.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>



namespace aegiscpp
{

class shard;
class aegis;

/**\todo Needs documentation
*/
struct resumed
{
    std::vector<std::string> _trace; /**<\todo Needs documentation */
    shard * _shard; /**<\todo Needs documentation */
    aegis * bot; /**<\todo Needs documentation */
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, resumed& m)
{
    if (j.count("_trace") && !j["_trace"].is_null())
        for (auto i : j["_trace"])
            m._trace.push_back(i);
}

/**\todo Needs documentation
*/
inline void to_json(nlohmann::json& j, const resumed& m)
{
    if (m._trace.size() > 0)
        for (auto i : m._trace)
            j["_trace"].push_back(i);
}

}

