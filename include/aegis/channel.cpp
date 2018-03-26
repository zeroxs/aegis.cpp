//
// channel.cpp
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

#include "aegis/config.hpp"
#include <spdlog/fmt/fmt.h>
#include <asio.hpp>
#include "aegis/channel.hpp"
#include "aegis/error.hpp"
#include "aegis/guild.hpp"
#include "aegis/shard.hpp"
#include "aegis/error.hpp"
#include "aegis/member.hpp"
#include "aegis/aegis.hpp"
#include "aegis/rest_reply.hpp"

namespace aegiscpp
{

using json = nlohmann::json;
using rest_limits::bucket_factory;

AEGIS_DECL guild & channel::get_guild() const
{
    if (_guild == nullptr)
        throw aegiscpp::exception("Guild not set", make_error_code(error::guild_not_found));
    return *_guild;
}

AEGIS_DECL aegis & channel::get_bot() const noexcept
{
    return *_bot;
}

AEGIS_DECL permission channel::perms()
{
    return permission(_guild->get_permissions(*_guild->self(), *this));
}


AEGIS_DECL std::future<rest_reply> channel::post_task(std::string path, std::string method, const nlohmann::json & obj)
{
    try
    {
        auto task(std::make_shared<std::packaged_task<rest_reply()>>(
            std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &ratelimit, channel_id, path, obj.dump(), method)));

        auto fut = task->get_future();

        get_bot().rest_service().post([t = std::move(task)]() { (*t)(); });

        return fut;
    }
    catch (nlohmann::json::type_error& e)
    {
        get_bot().log->critical("json::type_error channel::post_task() exception : {}", e.what());
    }
    catch (...)
    {
        get_bot().log->critical("Uncaught post_task exception");
    }
    return {};
}

AEGIS_DECL std::future<rest_reply> channel::post_emoji_task(std::string path, std::string method, const nlohmann::json & obj)
{
    auto task(std::make_shared<std::packaged_task<rest_reply()>>(
        std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &emojilimit, channel_id, path, obj.dump(), method)));

    auto fut = task->get_future();

    get_bot().rest_service().post([t = std::move(task)]() { (*t)(); });

    return fut;
}

AEGIS_DECL void channel::load_with_guild(guild & _guild, const json & obj, shard * _shard)
{
    snowflake channel_id = obj["id"];
    channel * _channel = _guild.get_channel_create(channel_id, _shard);

    try
    {
        //log->debug("Shard#{} : Channel[{}] created for guild[{}]", shard.m_shardid, channel_id, _channel.m_guild_id);
        if (!obj["name"].is_null()) _channel->name = obj["name"];
        _channel->position = obj["position"];
        _channel->type = static_cast<channel_type>(obj["type"].get<int>());// 0 = text, 2 = voice

        //voice channels
        if (obj.count("bitrate") && !obj["bitrate"].is_null())
        {
            _channel->bitrate = obj["bitrate"];
            _channel->user_limit = obj["user_limit"];
        }
        else
        {
            //not a voice channel, so has a topic field and last message id
            if (obj.count("topic") && !obj["topic"].is_null()) _channel->topic = obj["topic"];
            if (obj.count("last_message_id") && !obj["last_message_id"].is_null()) _channel->last_message_id = obj["last_message_id"];
        }

        if (obj.count("permission_overwrites") && !obj["permission_overwrites"].is_null())
        {
            json permission_overwrites = obj["permission_overwrites"];
            for (auto & permission : permission_overwrites)
            {
                uint32_t allow = permission["allow"];
                uint32_t deny = permission["deny"];
                snowflake p_id = permission["id"];
                std::string p_type = permission["type"];

                _channel->overrides[p_id].allow = allow;
                _channel->overrides[p_id].deny = deny;
                _channel->overrides[p_id].id = p_id;
                if (p_type == "role")
                    _channel->overrides[p_id].type = overwrite_type::Role;
                else
                    _channel->overrides[p_id].type = overwrite_type::User;
            }
        }

        //_channel.update_permission_cache();
    }
    catch (std::exception&e)
    {
        throw aegiscpp::exception(fmt::format("Shard#{} : Error processing channel[{}] of guild[{}] {}", _shard->get_id(), channel_id, _channel->guild_id, e.what()), make_error_code(error::channel_error));
    }
}

AEGIS_DECL rest_api channel::create_message(std::string content, int64_t nonce)
{
    std::error_code ec;
    if (_guild != nullptr)//probably a DM
        if (!perms().can_send_messages())
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    obj["content"] = content;

    if (nonce)
        obj["nonce"] = nonce;

    auto fut = post_task(fmt::format("/channels/{}/messages", channel_id), "POST", obj);
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::create_message_embed(std::string content, const json embed, int64_t nonce)
{
    std::error_code ec;
    if (_guild != nullptr)//probably a DM
        if (!perms().can_send_messages())
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;

    if (nonce)
        obj["nonce"] = nonce;

    auto fut = post_task(fmt::format("/channels/{}/messages", channel_id), "POST", obj);
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::edit_message(snowflake message_id, std::string content)
{
    json obj;
    obj["content"] = content;
    auto fut = post_task(fmt::format("/channels/{}/messages/{}", channel_id, message_id), "PATCH", obj);
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::edit_message_embed(snowflake message_id, std::string content, json embed)
{
    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;
    obj["content"] = content;
    auto fut = post_task(fmt::format("/channels/{}/messages/{}", channel_id, message_id), "PATCH", obj);
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

/**\todo can delete your own messages freely - provide separate function or keep history of messages
*/
AEGIS_DECL rest_api channel::delete_message(snowflake message_id)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}", guild_id, message_id), "DELETE");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::bulk_delete_message(std::vector<int64_t> messages)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    if (messages.size() < 2 || messages.size() > 100)
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj = messages;
    auto fut = post_task(fmt::format("/channels/{}/messages/bulk-delete", channel_id), "POST", obj);
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                                    std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                                    std::optional<std::vector<permission_overwrite>> permission_overwrites, std::optional<snowflake> parent_id)
{
    std::error_code ec;
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    if (name.has_value())
        obj["name"] = name.value();
    if (position.has_value())
        obj["position"] = position.value();
    if (topic.has_value())
        obj["topic"] = topic.value();
    if (nsfw.has_value())
        obj["nsfw"] = nsfw.value();
    if (bitrate.has_value())//voice only
        obj["bitrate"] = bitrate.value();
    if (user_limit.has_value())//voice only
        obj["user_limit"] = user_limit.value();
    if (permission_overwrites.has_value())//requires OWNER
    {
        if (_guild->owner_id != _guild->self()->member_id)
        {
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };
        }


        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : permission_overwrites.value())
        {
            obj["permission_overwrites"].push_back(p_ow);
        }
    }
    if (parent_id.has_value())//VIP only
        obj["parent_id"] = parent_id.value();

    auto fut = post_task(fmt::format("/channels/{}", channel_id), "PATCH", obj);
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::delete_channel()
{
    std::error_code ec;
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}", channel_id), "DELETE");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::create_reaction(snowflake message_id, std::string emoji_text)
{
    std::error_code ec;
    if (!perms().can_add_reactions())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text), "PUT");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::delete_own_reaction(snowflake message_id, std::string emoji_text)
{
    std::error_code ec;
    if (!perms().can_add_reactions())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text), "DELETE");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::delete_user_reaction(snowflake message_id, std::string emoji_text, snowflake member_id)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}/{}", channel_id, message_id, emoji_text, member_id), "DELETE");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

/**\todo Support query parameters
*  \todo before[snowflake], after[snowflake], limit[int]
*/
AEGIS_DECL rest_api channel::get_reactions(snowflake message_id, std::string emoji_text)
{
    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}", channel_id, message_id, emoji_text), "GET");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::delete_all_reactions(snowflake message_id)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions", channel_id, message_id), "DELETE");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type)
{
    std::error_code ec;
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    obj["allow"] = allow;
    obj["deny"] = deny;
    obj["type"] = type;
 
    auto fut = post_task(fmt::format("/channels/{}/permissions/{}", channel_id, overwrite_id), "PUT", obj);
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::get_channel_invites()
{
    std::error_code ec;
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}/invites", channel_id), "GET");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique)
{
    std::error_code ec;
    if (!perms().can_invite())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    if (max_age.has_value())
        obj["max_age"] = max_age.value();
    if (max_uses.has_value())
        obj["max_uses"] = max_uses.value();
    if (temporary.has_value())
        obj["temporary"] = temporary.value();
    if (unique.has_value())
        obj["unique"] = unique.value();

    auto fut = post_task(fmt::format("/channels/{}/invites", channel_id), "POST", obj);
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::delete_channel_permission(snowflake overwrite_id)
{
    std::error_code ec;
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/channels/{}/permissions/{}", channel_id, overwrite_id), "DELETE");
    return { ec, std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::trigger_typing_indicator()
{
    auto fut = post_task(fmt::format("/channels/{}/typing", channel_id));
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

AEGIS_DECL rest_api channel::get_pinned_messages()
{
    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

AEGIS_DECL rest_api channel::add_pinned_channel_message()
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

AEGIS_DECL rest_api channel::delete_pinned_channel_message()
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Will likely move to aegis class
* requires OAuth permissions to perform
*/
AEGIS_DECL rest_api channel::group_dm_add_recipient()//will go in aegiscpp::aegis
{
    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Will likely move to aegis class
* requires OAuth permissions to perform
*/
AEGIS_DECL rest_api channel::group_dm_remove_recipient()//will go in aegiscpp::aegis
{
    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

}

