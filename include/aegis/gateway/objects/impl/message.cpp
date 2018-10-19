//
// message.cpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/gateway/objects/message.hpp"
#include <nlohmann/json.hpp>
#include "aegis/core.hpp"
#include "aegis/futures.hpp"

namespace aegis
{

namespace gateway
{

namespace objects
{

AEGIS_DECL aegis::guild & message::get_guild()
{
    if (_guild == nullptr)
        _guild = internal::bot->find_guild(_guild_id);
    assert(_guild != nullptr);
    if (_guild == nullptr)
        throw exception("message::get_guild() _guild = nullptr", make_error_code(error::guild_not_found));
    return *_guild;
}

AEGIS_DECL aegis::channel & message::get_channel()
{
    if (_channel == nullptr)
        _channel = internal::bot->find_channel(_channel_id);
    assert(_channel != nullptr);
    if (_channel == nullptr)
        throw exception("message::get_channel() _channel == nullptr", make_error_code(error::channel_not_found));
    return *_channel;
}

#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL aegis::member & message::get_member()
{
    if (_member == nullptr)
        _member = internal::bot->find_member(_author_id);
    if (_member == nullptr)
    {
        if (_author_id == 0)
            throw exception("message::get_member() _member|_author_id == nullptr", make_error_code(error::member_not_found));

        _member = internal::bot->member_create(_author_id);
        _member->load_data(author);
    }
    return *_member;
}
#endif

AEGIS_DECL std::future<rest::rest_reply> message::delete_message(std::error_code & ec) noexcept
{
    populate_self();
    assert(_channel != nullptr);
    if (_channel == nullptr)
    {
        ec = make_error_code(error::general);
        return {};
    }

    ec = std::error_code();
    return get_channel().delete_message(_message_id);
}

AEGIS_DECL std::future<rest::rest_reply> message::delete_message()
{
    populate_self();
    assert(_channel != nullptr);
    std::error_code ec;
    if (_channel == nullptr)
    {
        ec = make_error_code(error::general);
        return {};
    }

    ec = std::error_code();
    auto res = get_channel().delete_message(ec, _message_id);
    if (ec)
        throw ec;
    return res;
}

AEGIS_DECL std::future<rest::rest_reply> message::edit(std::error_code & ec, const std::string & content) noexcept
{
    populate_self();
    assert(_channel != nullptr);
    if (_channel == nullptr)
    {
        ec = make_error_code(error::general);
        return {};
    }

    ec = std::error_code();
    return get_channel().edit_message(_message_id, content);
}

AEGIS_DECL std::future<rest::rest_reply> message::edit(const std::string & content)
{
    std::error_code ec;
    auto res = edit(ec, content);
    if (ec)
        throw ec;
    return res;
}

AEGIS_DECL std::future<rest::rest_reply> message::create_reaction(std::error_code & ec, const std::string & content) noexcept
{
    populate_self();
    assert(_channel != nullptr);
    if (_channel == nullptr)
    {
        ec = make_error_code(error::general);
        return {};
    }

    ec = std::error_code();
    return get_channel().create_reaction(_message_id, content);
}

AEGIS_DECL std::future<rest::rest_reply> message::create_reaction(const std::string & content)
{
    std::error_code ec;
    auto res = create_reaction(ec, content);
    if (ec)
        throw ec;
    return res;
}

AEGIS_DECL std::future<rest::rest_reply> message::delete_own_reaction(std::error_code & ec, const std::string & content) noexcept
{
    populate_self();
    assert(_channel != nullptr);
    if (_channel == nullptr)
    {
        ec = make_error_code(error::general);
        return {};
    }

    ec = std::error_code();
    return get_channel().delete_own_reaction(_message_id, content);
}

AEGIS_DECL std::future<rest::rest_reply> message::delete_own_reaction(const std::string & content)
{
    std::error_code ec;
    auto res = delete_own_reaction(ec, content);
    if (ec)
        throw ec;
    return res;
}

AEGIS_DECL std::future<rest::rest_reply> message::delete_user_reaction(std::error_code & ec, const std::string & content, const snowflake member_id) noexcept
{
    populate_self();
    assert(_channel != nullptr);
    if (_channel == nullptr)
    {
        ec = make_error_code(error::general);
        return {};
    }

    ec = std::error_code();
    return get_channel().delete_user_reaction(_message_id, content, member_id);
}

AEGIS_DECL std::future<rest::rest_reply> message::delete_user_reaction(const std::string & content, const snowflake member_id)
{
    std::error_code ec;
    auto res = delete_user_reaction(ec, content, member_id);
    if (ec)
        throw ec;
    return res;
}

AEGIS_DECL std::future<rest::rest_reply> message::delete_all_reactions(std::error_code & ec) noexcept
{
    populate_self();
    assert(_channel != nullptr);
    if (_channel == nullptr)
    {
        ec = make_error_code(error::general);
        return {};
    }

    ec = std::error_code();
    return get_channel().delete_all_reactions(_message_id);
}

AEGIS_DECL std::future<rest::rest_reply> message::delete_all_reactions()
{
    std::error_code ec;
    auto res = delete_all_reactions(ec);
    if (ec)
        throw ec;
    return res;
}

AEGIS_DECL void message::populate_self()
{
    if ((_guild == nullptr) && (_guild_id > 0))
        _guild = internal::bot->find_guild(_guild_id);
    if ((_channel == nullptr) && (_channel_id > 0))
        _channel = internal::bot->find_channel(_channel_id);
    if (_channel == nullptr)
        return;//fail
    if (_guild == nullptr)
        _guild = internal::bot->find_guild(_channel->get_guild_id());
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if ((_member == nullptr) && (_author_id > 0))
        _member = internal::bot->find_member(_author_id);
#endif
}

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, objects::message& m)
{
    m._message_id = j["id"];
    m._channel_id = j["channel_id"];
    if (j.count("author") && !j["author"].is_null())
    {
        m._author_id = j["author"]["id"];
        m.author = j["author"];
    }
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m._guild_id = j["guild_id"];
    if (j.count("content") && !j["content"].is_null())
        m._content = j["content"].get<std::string>();
    if (j.count("timestamp") && !j["timestamp"].is_null())
        m.timestamp = j["timestamp"].get<std::string>();
    if (j.count("edited_timestamp") && !j["edited_timestamp"].is_null())
        m.edited_timestamp = j["edited_timestamp"].get<std::string>();
    if (j.count("tts") && !j["tts"].is_null())
        m.tts = j["tts"];
    if (j.count("mention_everyone") && !j["mention_everyone"].is_null())
        m.mention_everyone = j["mention_everyone"];
    if (j.count("pinned") && !j["pinned"].is_null())
        m.pinned = j["pinned"];
    if (j.count("type") && !j["type"].is_null())
        m.type = j["type"];
    if (j.count("nonce") && !j["nonce"].is_null())
        m.nonce = j["nonce"];
    if (j.count("webhook_id") && !j["webhook_id"].is_null())
        m.webhook_id = j["webhook_id"].get<std::string>();
    if (j.count("mentions") && !j["mentions"].is_null())
        for (const auto & _mention : j["mentions"])
            m.mentions.push_back(_mention["id"]);
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & _mention_role : j["roles"])
            m.mention_roles.push_back(_mention_role);
    if (j.count("attachments") && !j["attachments"].is_null())
        for (const auto & _attachment : j["attachments"])
            m.attachments.push_back(_attachment);
    if (j.count("embeds") && !j["embeds"].is_null())
        for (const auto & _embed : j["embeds"])
            m.embeds.push_back(_embed);
    if (j.count("reactions") && !j["reactions"].is_null())
        for (const auto & _reaction : j["reactions"])
            m.reactions.push_back(_reaction);
}

AEGIS_DECL void to_json(nlohmann::json& j, const objects::message& m)
{
    j["id"] = m._message_id;
    j["content"] = m._content;
    j["timestamp"] = m.timestamp;
    j["edited_timestamp"] = m.edited_timestamp;
    j["tts"] = m.tts;
    j["mention_everyone"] = m.mention_everyone;
    j["pinned"] = m.pinned;
    j["type"] = m.type;
    if (m.nonce != 0)
        j["nonce"] = m.nonce;
    if (!m.webhook_id.empty())
        j["webhook_id"] = m.webhook_id;
    for (const auto & _mention : m.mentions)
        j["mentions"].push_back(_mention);
    for (const auto & _mention_role : m.mention_roles)
        j["roles"].push_back(_mention_role);
    for (const auto & _attachment : m.attachments)
        j["attachments"].push_back(_attachment);
    for (const auto & _embed : m.embeds)
        j["embeds"].push_back(_embed);
    if (!m.reactions.empty())
        for (const auto & _reaction : m.reactions)
            j["reactions"].push_back(_reaction);
}
/// \endcond

}

}

}
