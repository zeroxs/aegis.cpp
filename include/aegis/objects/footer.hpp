//
// footer.hpp
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
#include <string>
#include <vector>



namespace aegiscpp
{


/**\todo Needs documentation
*/
struct footer
{
    std::string text; /**<\todo Needs documentation */
    std::string icon_url; /**<\todo Needs documentation */
    std::string proxy_icon_url; /**<\todo Needs documentation */
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, footer& m)
{
    if (j.count("text") && !j["text"].is_null())
        m.text = j["text"];
    if (j.count("icon_url") && !j["icon_url"].is_null())
        m.icon_url = j["icon_url"];
    if (j.count("proxy_icon_url") && !j["proxy_icon_url"].is_null())
        m.proxy_icon_url = j["proxy_icon_url"];
}

/**\todo Needs documentation
*/
inline void to_json(nlohmann::json& j, const footer& m)
{
    j["text"] = m.text;
    j["icon_url"] = m.icon_url;
    j["proxy_icon_url"] = m.proxy_icon_url;
}

}

