//
// embed.hpp
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
#include "field.hpp"
#include "footer.hpp"
#include "image.hpp"
#include "thumbnail.hpp"
#include "video.hpp"
#include "provider.hpp"
#include <string>
#include <vector>



namespace aegiscpp
{

class embed
{
public:
    std::string title;
    std::string type;
    std::string description;
    std::string url;
    std::string timestamp;
    int32_t color = 0;
    footer footer_;
    image image_;
    thumbnail thumbnail_;
    video video_;
    provider provider_;
    std::vector<field> fields;
};

inline void from_json(const nlohmann::json& j, embed& m)
{
    if (j.count("title") && !j["title"].is_null())
        m.title = j["title"];
    if (j.count("type"))
        m.type = j["type"];
    if (j.count("description") && !j["description"].is_null())
        m.description = j["description"];
    if (j.count("url") && !j["url"].is_null())
        m.url = j["url"];
    if (j.count("timestamp") && !j["timestamp"].is_null())
        m.timestamp = j["timestamp"];
    if (j.count("color") && !j["color"].is_null())
        m.color = j["color"];
    if (j.count("footer") && !j["footer"].is_null())
        m.footer_ = j["footer"];
    if (j.count("image") && !j["image"].is_null())
        m.image_ = j["image"];
    if (j.count("thumbnail") && !j["thumbnail"].is_null())
        m.thumbnail_ = j["thumbnail"];
    if (j.count("video") && !j["video"].is_null())
        m.video_ = j["video"];
    if (j.count("provider") && !j["provider"].is_null())
        m.provider_ = j["provider"];
    if (j.count("fields") && !j["fields"].is_null())
        for (auto i : j["fields"])
            m.fields.push_back(i);
}
inline void to_json(nlohmann::json& j, const embed& m)
{
    j["title"] = m.title;
    j["type"] = m.type;
    j["description"] = m.description;
    j["url"] = m.url;
    j["timestamp"] = m.timestamp;
    j["color"] = m.color;
    j["footer"] = m.footer_;
    j["image"] = m.image_;
    j["thumbnail"] = m.thumbnail_;
    j["video"] = m.video_;
    j["provider"] = m.provider_;
    for (auto i : m.fields)
        j["fields"].push_back(i);
}

}

