//
// attachment.hpp
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
#include <nlohmann/json.hpp>
#include <string>



namespace aegiscpp
{



/**\todo Needs documentation
*/
struct attachment
{
    snowflake id; /**<\todo Needs documentation */
    std::string filename; /**<\todo Needs documentation */
    int32_t size = 0; /**<\todo Needs documentation */
    std::string url; /**<\todo Needs documentation */
    std::string proxy_url; /**<\todo Needs documentation */
    int32_t height = 0; /**<\todo Needs documentation */
    int32_t width = 0; /**<\todo Needs documentation */
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, attachment& m)
{
    if (j.count("id"))
        m.id = j["id"];
    if (j.count("filename"))
        m.filename = j["filename"];
    if (j.count("size"))
        m.size = j["size"];
    if (j.count("url"))
        m.url = j["url"];
    if (j.count("proxy_url"))
        m.proxy_url = j["proxy_url"];
    if (j.count("height") && !j["height"].is_null())
        m.height = j["height"];
    if (j.count("width") && !j["width"].is_null())
        m.width = j["width"];
}

/**\todo Needs documentation
*/
inline void to_json(nlohmann::json& j, const attachment& m)
{
    j["id"] = m.id;
    j["filename"] = m.filename;
    j["size"] = m.size;
    j["url"] = m.url;
    j["proxy_url"] = m.proxy_url;
    j["height"] = m.height;
    j["width"] = m.width;
}

}

