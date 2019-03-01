//
// message.hpp
// ***********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/shards/shard.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "aegis/gateway/objects/attachment.hpp"
#include "aegis/gateway/objects/embed.hpp"
#include "aegis/gateway/objects/reaction.hpp"
#include "aegis/gateway/objects/user.hpp"
#include <nlohmann/json.hpp>
#include "aegis/futures.hpp"

namespace aegis
{

namespace gateway
{

namespace objects
{

class message;

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, objects::message& m);

AEGIS_DECL void to_json(nlohmann::json& j, const objects::message& m);
/// \endcond

/// Type of message
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
    explicit message(const std::string & content, aegis::channel * _c, aegis::guild * _g) noexcept
        : _content(content)
        , _channel(_c)
        , _guild(_g)
    {
    }

    message(const std::string & _json, aegis::core * bot) noexcept
        : _bot(bot)
    {
        from_json(nlohmann::json::parse(_json), *this);
        populate_self();
    }

    message(const nlohmann::json & _json, aegis::core * bot) noexcept
        : _bot(bot)
    {
        from_json(_json, *this);
        populate_self();
    }

    message(aegis::core * bot) noexcept
        : _bot(bot)
    {
        populate_self();
    }

    /// Constructor for the message object
    /**
     * @param _shard Pointer to the shard this message is being handled by
     * @param channel_id Snowflake of channel this message belongs to
     * @param content String of the message sent
     */
    AEGIS_DECL void set_channel(aegis::channel * _c)
    {
        _channel = _c;
    }

    /// Constructor for the message object
    /**
     * @param _shard Pointer to the shard this message is being handled by
     * @param channel_id Snowflake of channel this message belongs to
     * @param content String of the message sent
     */
    AEGIS_DECL void set_guild(aegis::guild * _g)
    {
        _guild = _g;
    }

    message() = default;
    message & operator=(const message &) = default;
    message(const message&) = default;
    message(message && msg) = default;

    std::string timestamp; /**<\todo Needs documentation */
    std::string edited_timestamp; /**<\todo Needs documentation */
    bool tts = false; /**<\todo Needs documentation */
    bool mention_everyone = false; /**<\todo Needs documentation */
    std::vector<snowflake> mentions; /**<\todo Needs documentation */
    std::vector<snowflake> mention_roles; /**<\todo Needs documentation */
    std::vector<objects::attachment> attachments; /**<\todo Needs documentation */
    std::vector<objects::embed> embeds; /**<\todo Needs documentation */
    bool pinned = false; /**<\todo Needs documentation */
    std::vector<objects::reaction> reactions; /**<\todo Needs documentation */
    snowflake nonce; /**<\todo Needs documentation */
    std::string webhook_id; /**<\todo Needs documentation */
    objects::message_type type = Default; /**<\todo Needs documentation */
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

    snowflake get_channel_id() const noexcept
    {
        return _channel_id;
    }

    snowflake get_guild_id() const noexcept
    {
        return _guild_id;
    }

    snowflake get_author_id() const noexcept
    {
        return _author_id;
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
        return _user != nullptr || _author_id != 0;
    }
#endif

    AEGIS_DECL aegis::guild & get_guild();

    AEGIS_DECL aegis::channel & get_channel();

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    AEGIS_DECL aegis::member & get_member();
#endif

    AEGIS_DECL aegis::future<rest::rest_reply> delete_message();

    AEGIS_DECL aegis::future<message> edit(const std::string & content);

    AEGIS_DECL aegis::future<rest::rest_reply> create_reaction(const std::string & content);

    AEGIS_DECL aegis::future<rest::rest_reply> delete_own_reaction(const std::string & content);

    AEGIS_DECL aegis::future<rest::rest_reply> delete_user_reaction(const std::string & content, const snowflake member_id);

    AEGIS_DECL aegis::future<rest::rest_reply> delete_all_reactions();

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

private:
    friend AEGIS_DECL void from_json(const nlohmann::json& j, objects::message& m);
    friend AEGIS_DECL void to_json(nlohmann::json& j, const objects::message& m);
    friend class core;

    AEGIS_DECL void populate_self();

    std::string _content;/**< String of the message contents */
    aegis::channel * _channel = nullptr;/**< Pointer to the channel this message belongs to */
    aegis::guild * _guild = nullptr;/**< Pointer to the guild this message belongs to */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    aegis::member * _member = nullptr;/**< Pointer to the author of this message */
#endif
    aegis::core * _bot = nullptr;
    snowflake _message_id = 0; /**< snowflake of the message */
    snowflake _channel_id = 0; /**< snowflake of the channel this message belongs to */
    snowflake _guild_id = 0; /**< snowflake of the guild this message belongs to */
    snowflake _author_id = 0; /**< snowflake of the author of this message */
};

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/objects/impl/message.cpp"
#endif
