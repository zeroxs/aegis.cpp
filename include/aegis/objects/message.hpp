//
// message.hpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/channel.hpp"
#include "aegis/shards/shard.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "attachment.hpp"
#include "embed.hpp"
#include "reaction.hpp"
#include "user.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "aegis/guild.hpp"

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
enum message_type
{
    Default = 0,
    RecipientAdd = 1,
    RecipientRemove = 2,
    Call = 3,
    ChannelNameChange = 4,
    ChannelIconChange = 5,
    ChannelPinnedMessage = 6,
    GuildMemberJoin = 7
};

#if !defined(AEGIS_CXX17)
template< class T >
struct V
{
    static core * bot;
};

template< class T >
core * V<T>::bot = nullptr;

using BOT = V<void>;
#endif

/**\todo Needs documentation
 */
class message
{
public:
    /// Constructor for the message object
    /**
     * @param _shard Pointer to the shard this message is being handled by
     * @param _c Pointer of channel object
     * @param _g Pointer of guild object
     * @param content String of the message sent
     */
    explicit message(const std::string & content, channel * _c, guild * _g) noexcept
        : _content(content)
        , _channel(_c)
        , _guild(_g)
    {
    }

    /// Constructor for the message object
    /**
     * @param _shard Pointer to the shard this message is being handled by
     * @param channel_id Snowflake of channel this message belongs to
     * @param content String of the message sent
     */
    void set_channel(channel * _c)
    {
        _channel = _c;
    }

    /// Constructor for the message object
    /**
     * @param _shard Pointer to the shard this message is being handled by
     * @param channel_id Snowflake of channel this message belongs to
     * @param content String of the message sent
     */
    void set_guild(guild * _g)
    {
        _guild = _g;
    }

    explicit message() = default;

    std::string timestamp; /**<\todo Needs documentation */
    std::string edited_timestamp; /**<\todo Needs documentation */
    bool tts = false; /**<\todo Needs documentation */
    bool mention_everyone = false; /**<\todo Needs documentation */
    std::vector<snowflake> mentions; /**<\todo Needs documentation */
    std::vector<snowflake> mention_roles; /**<\todo Needs documentation */
    std::vector<attachment> attachments; /**<\todo Needs documentation */
    std::vector<embed> embeds; /**<\todo Needs documentation */
    bool pinned = false; /**<\todo Needs documentation */
    std::vector<reaction> reactions; /**<\todo Needs documentation */
    snowflake nonce; /**<\todo Needs documentation */
    std::string webhook_id; /**<\todo Needs documentation */
    message_type type = Default; /**<\todo Needs documentation */
    user author; /**< author object for this message */

    bool is_bot() const noexcept
    {
        return author.is_bot();
    }

    bool is_webhook() const noexcept
    {
        return author.is_webhook();
    }

    const std::string & get_content() const noexcept
    {
        return _content;
    }

    void set_content(const std::string & content) noexcept
    {
        _content = content;
    }

    snowflake get_id() const noexcept
    {
        return _message_id;
    }

    bool has_guild() const noexcept
    {
        return _guild != nullptr || _guild_id != 0;
    }

    bool has_channel() const noexcept
    {
        return _channel != nullptr || _channel_id != 0;
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    bool has_member() const noexcept
    {
        return _member != nullptr || _author_id != 0;
    }
#endif

    guild & get_guild()
    {
        if (_guild == nullptr)
#if defined(AEGIS_CXX17)
            _guild = bot->find_guild(_guild_id);
#else
            _guild = BOT::bot->find_guild(_guild_id);
#endif
        assert(_guild != nullptr);
        if (_guild == nullptr)
            throw exception("message::get_guild() _guild = nullptr", make_error_code(error::guild_not_found));
        return *_guild;
    }

    channel & get_channel()
    {
        if (_channel == nullptr)
#if defined(AEGIS_CXX17)
            _channel = bot->find_channel(_channel_id);
#else
            _channel = BOT::bot->find_channel(_channel_id);
#endif
        assert(_channel != nullptr);
        if (_channel == nullptr)
            throw exception("message::get_channel() _channel == nullptr", make_error_code(error::channel_not_found));
        return *_channel;
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    member & get_member()
    {
        if (_member == nullptr)
#if defined(AEGIS_CXX17)
            _member = bot->find_member(_author_id);
#else
            _member = BOT::bot->find_member(_author_id);
#endif
        if (_member == nullptr)
        {
            if (_author_id == 0)
                throw exception("message::get_member() _member|_author_id == nullptr", make_error_code(error::member_not_found));

#if defined(AEGIS_CXX17)
            _member = bot->member_create(_author_id);
#else
            _member = BOT::bot->member_create(_author_id);
#endif
            _member->load_data(author);
        }
        return *_member;
    }
#endif

    std::future<rest::rest_reply> delete_message(std::error_code & ec) noexcept
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

    std::future<rest::rest_reply> delete_message()
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

    std::future<rest::rest_reply> edit(std::error_code & ec, const std::string & content) noexcept
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

    std::future<rest::rest_reply> edit(const std::string & content)
    {
        std::error_code ec;
        auto res = edit(ec, content);
        if (ec)
            throw ec;
        return res;
    }

    std::future<rest::rest_reply> create_reaction(std::error_code & ec, const std::string & content) noexcept
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

    std::future<rest::rest_reply> create_reaction(const std::string & content)
    {
        std::error_code ec;
        auto res = create_reaction(ec, content);
        if (ec)
            throw ec;
        return res;
    }

    std::future<rest::rest_reply> delete_own_reaction(std::error_code & ec, const std::string & content) noexcept
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

    std::future<rest::rest_reply> delete_own_reaction(const std::string & content)
    {
        std::error_code ec;
        auto res = delete_own_reaction(ec, content);
        if (ec)
            throw ec;
        return res;
    }

    std::future<rest::rest_reply> delete_user_reaction(std::error_code & ec, const std::string & content, const snowflake member_id) noexcept
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

    std::future<rest::rest_reply> delete_user_reaction(const std::string & content, const snowflake member_id)
    {
        std::error_code ec;
        auto res = delete_user_reaction(ec, content, member_id);
        if (ec)
            throw ec;
        return res;
    }

    std::future<rest::rest_reply> delete_all_reactions(std::error_code & ec) noexcept
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

    std::future<rest::rest_reply> delete_all_reactions()
    {
        std::error_code ec;
        auto res = delete_all_reactions(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Obtain the relevant snowflakes related to this message
    /**
     * Returns { _channel_id, _guild_id, _message_id, _author_id }
     * Some may be 0 such as guild for a DM or author for a webhook
     * @returns std::tuple<snowflake, snowflake, snowflake, snowflake>
     */ 
    std::tuple<snowflake, snowflake, snowflake, snowflake> get_related_ids() const noexcept
    {
        return std::tuple<snowflake, snowflake, snowflake, snowflake>{ _channel_id, _guild_id, _message_id, _author_id };
    };

#if defined(AEGIS_CXX17)
    static inline core * bot = nullptr;
#endif

private:
    friend void from_json(const nlohmann::json& j, message& m);
    friend void to_json(nlohmann::json& j, const message& m);
    friend class core;

    //static void set_bot(core * b) { bot = b; }
#if defined(AEGIS_CXX17)
    void populate_self()
    {
        if ((_guild == nullptr) && (_guild_id > 0))
            _guild = bot->find_guild(_guild_id);
        if ((_channel == nullptr) && (_channel_id > 0))
            _channel = bot->find_channel(_channel_id);
        if (_guild == nullptr)
            _guild = bot->find_guild(_channel->get_guild_id());
#if !defined(AEGIS_DISABLE_ALL_CACHE)
        if ((_member == nullptr) && (_author_id > 0))
            _member = bot->find_member(_author_id);
#endif
    };
#else
    void populate_self()
    {
        if ((_guild == nullptr) && (_guild_id > 0))
            _guild = BOT::bot->find_guild(_guild_id);
        if ((_channel == nullptr) && (_channel_id > 0))
            _channel = BOT::bot->find_channel(_channel_id);
        if (_channel == nullptr)
            return;//fail
        if (_guild == nullptr)
            _guild = BOT::bot->find_guild(_channel->get_guild_id());
#if !defined(AEGIS_DISABLE_ALL_CACHE)
        if ((_member == nullptr) && (_author_id > 0))
            _member = BOT::bot->find_member(_author_id);
#endif
    };
#endif
    std::string _content;/**< String of the message contents */
    channel * _channel = nullptr;/**< Pointer to the channel this message belongs to */
    guild * _guild = nullptr;/**< Pointer to the guild this message belongs to */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    member * _member = nullptr;/**< Pointer to the author of this message */
#endif
    snowflake _message_id = 0; /**< snowflake of the message */
    snowflake _channel_id = 0; /**< snowflake of the channel this message belongs to */
    snowflake _guild_id = 0; /**< snowflake of the guild this message belongs to */
    snowflake _author_id = 0; /**< snowflake of the author of this message */
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, message& m)
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

/**\todo Needs documentation
 */
inline void to_json(nlohmann::json& j, const message& m)
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

}

}

}
