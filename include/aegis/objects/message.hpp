//
// message.hpp
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
#include "../channel.hpp"
#include "../guild.hpp"
#include "../shard.hpp"
#include "../rest_reply.hpp"
#include "attachment.hpp"
#include "embed.hpp"
#include "reaction.hpp"
#include "user.hpp"
#include "../shard.hpp"
#include "../aegis.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>



namespace aegiscpp
{

class member;
class channel;

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

/**\todo Needs documentation
*/
class message
{
public:
    /// Constructor for the message object
    /**
    * @param _shard Pointer to the shard this message is being handled by
    *
    * @param channel_id Snowflake of channel this message belongs to
    *
    * @param content String of the message sent
    */
    explicit message(shard * _shard, snowflake channel_id, std::string content)
        : _channel_id(channel_id)
        , _initialized(true)
        , _shard(_shard)
        , _content(content)
        , _guild(nullptr)
        , _channel(nullptr)
    {
        _channel = get_shard().get_bot().get_channel(_channel_id);

        if (_channel != nullptr)
        {
            _guild_id = _channel->guild_id;
            if (_channel->_guild != nullptr)
            {
                _guild = get_shard().get_bot().get_guild(_guild_id);
                _initialized = true;
                return;
            }
        }
    }

    explicit message()
    {

    }

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

    bool is_valid() const noexcept
    {
        return _initialized;
    }

    std::string_view get_content() const noexcept
    {
        return _content;
    }

    void set_content(std::string & content) noexcept
    {
        _content = content;
    }

    snowflake get_id() const noexcept
    {
        return _message_id;
    }

    shard & get_shard() const
    {
        if (_shard == nullptr) throw aegiscpp::exception("message::get_shard() nullptr");// This should -never- throw
        return *_shard;
    }

    guild & get_guild() const
    {
        if (_guild == nullptr)
        {
            if (_guild_id == 0) throw aegiscpp::exception("message::get_guild() id = 0", make_error_code(error::guild_not_found));// This should -never- throw
            return *get_shard().get_bot().get_guild(_guild_id);
        }
        return *_guild;
    }

    channel & get_channel() const
    {
        if (_channel == nullptr)
        {
            if (_channel_id == 0) throw aegiscpp::exception("message::get_channel() id = 0", make_error_code(error::channel_not_found));// This should -never- throw
            return *get_guild().get_channel(_channel_id);
        }
        return *_channel;
    }

    rest_api edit(const std::string & content) noexcept
    {
        if (!_initialized)
            return { make_error_code(error::general),std::make_optional<std::future<rest_reply>>() };
        return get_channel().edit_message(_message_id, content);
    }

    rest_api create_reaction(const std::string & content) noexcept
    {
        if (!_initialized)
            return { make_error_code(error::general),std::make_optional<std::future<rest_reply>>() };
        return get_channel().create_reaction(_message_id, content);
    }

    rest_api delete_own_reaction(const std::string & content) noexcept
    {
        if (!_initialized)
            return { make_error_code(error::general),std::make_optional<std::future<rest_reply>>() };
        return get_channel().delete_own_reaction(_message_id, content);
    }

    rest_api delete_user_reaction(const std::string & content, snowflake member_id) noexcept
    {
        if (!_initialized)
            return { make_error_code(error::general),std::make_optional<std::future<rest_reply>>() };
        return get_channel().delete_user_reaction(_message_id, content, member_id);
    }

    rest_api delete_all_reactions() noexcept
    {
        if (!_initialized)
            return { make_error_code(error::general),std::make_optional<std::future<rest_reply>>() };
        return get_channel().delete_all_reactions(_message_id);
    }

    /// Obtain the relevant snowflakes related to this message
    /**
     * Returns { _channel_id, _guild_id, _message_id, _author_id }
     * Some may be 0 such as guild for a DM or author for a webhook
     * @returns std::tuple<snowflake, snowflake, snowflake, snowflake>
     */ 
    std::tuple<snowflake, snowflake, snowflake, snowflake> get_related_ids()
    {
        return { _channel_id, _guild_id, _message_id, _author_id };
    };

    /// Initializes the pointers for this message according to ids provided
    /**
    * @param _shard Pointer to shard object
    */
    void init(shard * _shard)
    {
        if (_shard == nullptr)
            return;
        this->_shard = _shard;

        _channel = get_shard().get_bot().get_channel(_channel_id);

        if (_channel != nullptr)
        {
            _guild_id = _channel->get_guild_id();
            if (_channel->_guild != nullptr)
            {
                _guild = get_shard().get_bot().get_guild(_guild_id);
                _initialized = true;
                return;
            }
        }
    }

private:
    friend void from_json(const nlohmann::json& j, message& m);
    friend void to_json(nlohmann::json& j, const message& m);
    friend class aegis;

    bool _initialized = false;
    shard * _shard = nullptr;/**< Pointer to the shard that handled this message */
    std::string _content;/**< String of the message contents */
    channel * _channel = nullptr;/**< Pointer to the channel this message belongs to */
    guild * _guild = nullptr;/**< Pointer to the guild this message belongs to */
    member * _member = nullptr;/**< Pointer to the author of this message */
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
    if (j.count("content") && !j["content"].is_null())
        m._content = j["content"];
    if (j.count("timestamp") && !j["timestamp"].is_null())
        m.timestamp = j["timestamp"];
    if (j.count("edited_timestamp") && !j["edited_timestamp"].is_null())
        m.edited_timestamp = j["edited_timestamp"];
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
        m.webhook_id = j["webhook_id"];
    if (j.count("mentions") && !j["mentions"].is_null())
        for (const auto& i : j["mentions"])
            m.mentions.push_back(i["id"]);
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto& i : j["roles"])
            m.mention_roles.push_back(i);
    if (j.count("attachments") && !j["attachments"].is_null())
        for (const auto& i : j["attachments"])
            m.attachments.push_back(i);
    if (j.count("embeds") && !j["embeds"].is_null())
        for (const auto& i : j["embeds"])
            m.embeds.push_back(i);
    if (j.count("reactions") && !j["reactions"].is_null())
        for (const auto& i : j["reactions"])
            m.reactions.push_back(i);
    bool _initialized = true;
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
    if (m.webhook_id.size() > 0)
        j["webhook_id"] = m.webhook_id;
    for (auto i : m.mentions)
        j["mentions"].push_back(i);
    for (auto i : m.mention_roles)
        j["roles"].push_back(i);
    for (auto i : m.attachments)
        j["attachments"].push_back(i);
    for (auto i : m.embeds)
        j["embeds"].push_back(i);
    if (m.reactions.size() > 0)
        for (auto i : m.reactions)
            j["reactions"].push_back(i);
}



}

