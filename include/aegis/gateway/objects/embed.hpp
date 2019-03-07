//
// embed.hpp
// *********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/field.hpp"
#include "aegis/gateway/objects/footer.hpp"
#include "aegis/gateway/objects/image.hpp"
#include "aegis/gateway/objects/thumbnail.hpp"
#include "aegis/gateway/objects/video.hpp"
#include "aegis/gateway/objects/provider.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
class embed
{
public:
    /// Adds a new embed field
    /**
     * @param name Name of the field
     * @param value Text to be shown within field
     * @param is_inline Sets whether the field is inline
     */
    embed& fields(const std::vector<objects::field>& flds) noexcept
    {
        _fields = flds;
        return *this;
    }

    /// Sets the title of the embed
    /**
     * @param str Title to set
     */
    embed& title(const std::string& str) noexcept
    {
        _title = str;
        return *this;
    }

    /// Sets the footer of the embed
    /**
     * @param str Footer to set
     */
    embed& footer(const objects::footer ftr) noexcept
    {
        _footer = ftr;
        return *this;
    }

    /// Sets the description of the embed
    /**
     * @param str Description to set
     */
    embed& description(const std::string& str) noexcept
    {
        _description = str;
        return *this;
    }

    /// Sets the url of the embed
    /**
     * @param str Url to set
     */
    embed& url(const std::string& str) noexcept
    {
        _url = str;
        return *this;
    }

    /// Sets the timestamp of the embed
    /**
     * @param str Timestamp to set
     */
    embed& timestamp(const std::string& str) noexcept
    {
        _timestamp = str;
        return *this;
    }

    /// Sets the color of the embed
    /**
     * @param clr Color to set
     */
    embed& color(const int32_t clr) noexcept
    {
        _color = clr;
        return *this;
    }

    /// Sets the thumbnail of the embed
    /**
     * @param tn Thumbnail to set
     */
    embed& thumbnail(const objects::thumbnail& tn) noexcept
    {
        _thumbnail = tn;
        return *this;
    }

    /// Sets the image of the embed
    /**
     * @param img Image to set
     */
    embed& image(const objects::image& img) noexcept
    {
        _image = img;
        return *this;
    }

    /// Get the fields
    std::vector<objects::field>& fields() noexcept
    {
        return _fields;
    }

    /// Get the title
    std::string& title() noexcept
    {
        return _title;
    }

    /// Get the footer
    objects::footer& footer() noexcept
    {
        return _footer;
    }

    /// Get the description
    std::string& description() noexcept
    {
        return _description;
    }

    /// Get the url
    std::string& url() noexcept
    {
        return _url;
    }

    /// Get the timestamp
    std::string& timestamp() noexcept
    {
        return _timestamp;
    }

    /// Get the color
    int32_t& color() noexcept
    {
        return _color;
    }

    /// Get the thumbnail
    objects::thumbnail& thumbnail() noexcept
    {
        return _thumbnail;
    }

    /// Get the image
    objects::image& image() noexcept
    {
        return _image;
    }

private:
    // Combined Limit: 6000
    std::string _title; /**<\todo Needs documentation */ // Limit: 256
    std::string _type; /**<\todo Needs documentation */
    std::string _description; /**<\todo Needs documentation */ // Limit: 2048
    std::string _url; /**<\todo Needs documentation */
    std::string _timestamp; /**<\todo Needs documentation */
    int32_t _color = 0; /**<\todo Needs documentation */
    objects::footer _footer; /**<\todo Needs documentation */ // Limit: 2048
    objects::image _image; /**<\todo Needs documentation */
    objects::thumbnail _thumbnail; /**<\todo Needs documentation */
    objects::video _video; /**<\todo Needs documentation */
    objects::provider _provider; /**<\todo Needs documentation */
    std::vector<objects::field> _fields; /**<\todo Needs documentation */ // Limit: 25 name:256 value:1024
    friend void from_json(const nlohmann::json& j, embed& m);
    friend void to_json(nlohmann::json& j, const embed& m);
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, embed& m)
{
    if (j.count("title") && !j["title"].is_null())
        m._title = j["title"].get<std::string>();
    if (j.count("type"))
        m._type = j["type"].get<std::string>();
    if (j.count("description") && !j["description"].is_null())
        m._description = j["description"].get<std::string>();
    if (j.count("url") && !j["url"].is_null())
        m._url = j["url"].get<std::string>();
    if (j.count("timestamp") && !j["timestamp"].is_null())
        m._timestamp = j["timestamp"].get<std::string>();
    if (j.count("color") && !j["color"].is_null())
        m._color = j["color"];
    if (j.count("footer") && !j["footer"].is_null())
        m._footer = j["footer"];
    if (j.count("image") && !j["image"].is_null())
        m._image = j["image"];
    if (j.count("thumbnail") && !j["thumbnail"].is_null())
        m._thumbnail = j["thumbnail"];
    if (j.count("video") && !j["video"].is_null())
        m._video = j["video"];
    if (j.count("provider") && !j["provider"].is_null())
        m._provider = j["provider"];
    if (j.count("fields") && !j["fields"].is_null())
        for (const auto& _field : j["fields"])
            m._fields.push_back(_field);
}

inline void to_json(nlohmann::json & j, const embed & m)
{
    j["title"] = m._title;
    j["type"] = m._type;
    j["description"] = m._description;
    j["url"] = m._url;
    j["timestamp"] = m._timestamp;
    j["color"] = m._color;
    j["footer"] = m._footer;
    j["image"] = m._image;
    j["thumbnail"] = m._thumbnail;
    j["video"] = m._video;
    j["provider"] = m._provider;
    for (const auto& _field : m._fields)
        j["fields"].push_back(_field);
}
/// \endcond

}

}

}
