//
// channel_impl.hpp
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

#include "config.hpp"
#include "error.hpp"

namespace aegiscpp
{

using json = nlohmann::json;
using rest_limits::bucket_factory;

inline guild & channel::get_guild()
{
    return *_guild;
}

inline permission channel::perms()
{
    return permission(_guild->get_permissions(*_guild->self(), *this));
}


inline std::shared_future<rest_reply> channel::post_task(std::string path, std::string method, std::string obj)
{
    auto task(std::make_shared<std::packaged_task<rest_reply()>>(
        std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &ratelimit, channel_id, path, obj, method)));

    auto fut = task->get_future().share();

    get_guild().state->core->rest_scheduler->post([task]() { (*task)(); });

    return fut;
}

inline std::shared_future<rest_reply> channel::post_emoji_task(std::string path, std::string method, std::string obj)
{
    auto task(std::make_shared<std::packaged_task<rest_reply()>>(
        std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &emojilimit, channel_id, path, obj, method)));

    auto fut = task->get_future().share();

    get_guild().state->core->rest_scheduler->post([task]() { (*task)(); });

    return fut;
}

inline void channel::load_with_guild(guild & _guild, const json & obj, shard * _shard)
{
    snowflake channel_id = obj["id"];
    channel * _channel = _guild.get_channel_create(channel_id, _shard);
    _channel->guild_id = _guild.guild_id;

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
        log->error("Shard#{} : Error processing channel[{}] of guild[{}] {}", _shard->get_id(), channel_id, _channel->guild_id, e.what());
    }
}

inline bool channel::create_debug_message(std::string content)
{
    json obj;
    obj["content"] = content;
    ratelimit.push(channel_id, fmt::format("/channels/{}/messages", channel_id), obj.dump(), "POST");
    return true;
}

inline rest_api channel::create_message(std::string content)
{
    std::error_code ec;
    if (_guild != nullptr)//probably a DM
        if (!perms().can_send_messages())
            return { make_error_code(error::no_permission), {} };

    json obj;
    obj["content"] = content;

    auto fut = post_task(fmt::format("/channels/{}/messages", channel_id), "POST", obj.dump());
    return { ec, fut };

//     auto task(std::make_shared<std::packaged_task<rest_reply()>>(
//         std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &ratelimit, channel_id, fmt::format("/channels/{}/messages", channel_id), obj.dump(), "POST")));
// 
//     auto fut = task->get_future().share();
// 
//     get_guild().state->core->rest_scheduler->post([task]() { (*task)(); });
// 
//     return { ec, fut };




//     std::promise<rest_reply> prom;
//     auto fut = prom.get_future();
//     prom.set_value(rest_reply());
// 
//     get_guild().state->core->rest_scheduler->post([prom = std::move(prom), channel_id = channel_id, msg = obj.dump(), ratelimit = &ratelimit, method = "POST"s]() mutable
//     {
//         //std::promise<rest_reply> prom2;
//         prom.set_value(std::move(ratelimit->do_async(channel_id, fmt::format("/channels/{}/messages", channel_id), msg, method)));
//     });
/*
//     if (_guild != nullptr)//probably a DM
//         if (!permission(_guild->get_permissions(*_guild->self(), *this)).can_send_messages())
//             return { false, "Cannot send messages" };
//     std::future<rest_reply> ad;

    //std::packaged_task<rest_reply(snowflake, std::string, std::string, std::string)> task(std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &ratelimit, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    json obj;
    obj["content"] = content;

    auto task(std::make_shared<std::packaged_task<rest_reply(snowflake, std::string, std::string, std::string)>>(std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &ratelimit, channel_id, fmt::format("/channels/{}/messages", channel_id), obj.dump(), "POST")));
//     std::packaged_task<rest_reply(snowflake, std::string, std::string, std::string)> task([&, msg = obj.dump()]()
//     {
//         return ratelimit.do_async(channel_id, fmt::format("/channels/{}/messages", channel_id), msg, "POST");
//     });
// 
//     std::function<rest_reply(void)> f = std::bind(std::forward(task));

    auto fut = task->get_future();
    //get_guild().state->core->rest_scheduler->post(std::bind(std::move(task), channel_id, fmt::format("/channels/{}/messages", channel_id), obj.dump(), "POST"));
    get_guild().state->core->rest_scheduler->post([task, channel_id = channel_id, msg = obj.dump()]()
    {
        return (*task.get())(channel_id, fmt::format("/channels/{}/messages", channel_id), msg, "POST");
    });
    return fut;*/
}

inline rest_api channel::create_message_embed(std::string content, const json embed)
{
    std::error_code ec;
    if (_guild != nullptr)//probably a DM
        if (!perms().can_send_messages())
            return { make_error_code(error::no_permission),{} };

    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;

    auto fut = post_task(fmt::format("/channels/{}/messages", channel_id), "POST", obj.dump());
    return { ec, fut };
}

inline rest_api channel::edit_message(snowflake message_id, std::string content)
{
    json obj;
    obj["content"] = content;
    auto fut = post_task(fmt::format("/channels/{}/messages/{}", channel_id, message_id), "PATCH", obj.dump());
    return { std::error_code(), fut };
}

inline rest_api channel::edit_message_embed(snowflake message_id, std::string content, json embed)
{
    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;
    obj["content"] = content;
    auto fut = post_task(fmt::format("/channels/{}/messages/{}", channel_id, message_id), "PATCH", obj.dump());
    return { std::error_code(), fut };
}

/**\todo can delete your own messages freely - provide separate function or keep history of messages
*/
inline rest_api channel::delete_message(snowflake message_id)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}", guild_id, message_id), "DELETE");
    return { ec, fut };
}

inline rest_api channel::bulk_delete_message(std::vector<int64_t> messages)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission),{} };

    if (messages.size() < 2 || messages.size() > 100)
        return { make_error_code(error::no_permission),{} };

    json obj = messages;
    auto fut = post_task(fmt::format("/channels/{}/messages/bulk-delete", channel_id), "POST", obj.dump());
    return { ec, fut };
}

inline rest_api channel::modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                                    std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                                    std::optional<std::vector<permission_overwrite>> permission_overwrites, std::optional<snowflake> parent_id)
{
    std::error_code ec;
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission),{} };

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
            ec = make_error_code(error::no_permission);
            return {};
        }


        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : permission_overwrites.value())
        {
            obj["permission_overwrites"].push_back(p_ow);
        }
    }
    if (parent_id.has_value())//VIP only
        obj["parent_id"] = parent_id.value();

    auto fut = post_task(fmt::format("/channels/{}", channel_id), "PATCH", obj.dump());
    return { ec, fut };
}

inline rest_api channel::delete_channel()
{
    std::error_code ec;
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}", channel_id), "DELETE");
    return { ec, fut };
}

inline rest_api channel::create_reaction(snowflake message_id, std::string emoji_text)
{
    std::error_code ec;
    if (!perms().can_add_reactions())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text), "PUT");
    return { ec, fut };
}

inline rest_api channel::delete_own_reaction(snowflake message_id, std::string emoji_text)
{
    std::error_code ec;
    if (!perms().can_add_reactions())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text), "DELETE");
    return { ec, fut };
}

inline rest_api channel::delete_user_reaction(snowflake message_id, std::string emoji_text, snowflake member_id)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}/{}", channel_id, message_id, emoji_text, member_id), "DELETE");
    return { ec, fut };
}

/**\todo Support query parameters
*  \todo before[snowflake], after[snowflake], limit[int]
*/
inline rest_api channel::get_reactions(snowflake message_id, std::string emoji_text)
{
    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions/{}", channel_id, message_id, emoji_text), "GET");
    return { std::error_code(), fut };
}

inline rest_api channel::delete_all_reactions(snowflake message_id)
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}/messages/{}/reactions", channel_id, message_id), "DELETE");
    return { ec, fut };
}

inline rest_api channel::edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type)
{
    std::error_code ec;
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission),{} };

    json obj;
    obj["allow"] = allow;
    obj["deny"] = deny;
    obj["type"] = type;
 
    auto fut = post_task(fmt::format("/channels/{}/permissions/{}", channel_id, overwrite_id), "PUT", obj.dump());
    return { ec, fut };
}

inline rest_api channel::get_channel_invites()
{
    std::error_code ec;
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}/invites", channel_id), "GET");
    return { ec, fut };
}

inline rest_api channel::create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique)
{
    std::error_code ec;
    if (!perms().can_invite())
        return { make_error_code(error::no_permission),{} };

    json obj;
    if (max_age.has_value())
        obj["max_age"] = max_age.value();
    if (max_uses.has_value())
        obj["max_uses"] = max_uses.value();
    if (temporary.has_value())
        obj["temporary"] = temporary.value();
    if (unique.has_value())
        obj["unique"] = unique.value();

    auto fut = post_task(fmt::format("/channels/{}/invites", channel_id), "POST", obj.dump());
    return { ec, fut };
}

inline rest_api channel::delete_channel_permission(snowflake overwrite_id)
{
    std::error_code ec;
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission),{} };

    auto fut = post_task(fmt::format("/channels/{}/permissions/{}", channel_id, overwrite_id), "DELETE");
    return { ec, fut };
}

inline rest_api channel::trigger_typing_indicator()
{
    auto fut = post_task(fmt::format("/channels/{}/typing", channel_id));
    return { std::error_code(), fut };
}

inline rest_api channel::get_pinned_messages()
{
    return { make_error_code(error::not_implemented),{} };
}

inline rest_api channel::add_pinned_channel_message()
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission),{} };

    return { make_error_code(error::not_implemented),{} };
}

inline rest_api channel::delete_pinned_channel_message()
{
    std::error_code ec;
    if (!perms().can_manage_messages())
        return { make_error_code(error::no_permission),{} };

    return { make_error_code(error::not_implemented),{} };
}

/**\todo Will likely move to aegis class
*/
inline rest_api channel::group_dm_add_recipient()//will go in aegis::aegis
{
    return { make_error_code(error::not_implemented),{} };
}

/**\todo Will likely move to aegis class
*/
inline rest_api channel::group_dm_remove_recipient()//will go in aegis::aegis
{
    return { make_error_code(error::not_implemented),{} };
}

}

