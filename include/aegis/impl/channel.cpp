//
// channel.cpp
// ***********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/channel.hpp"
#include "aegis/core.hpp"
#include "aegis/error.hpp"
#include "aegis/guild.hpp"
#include "aegis/shards/shard.hpp"
#include "aegis/user.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "aegis/ratelimit/ratelimit.hpp"
#include "aegis/gateway/objects/message.hpp"
#include "aegis/gateway/objects/messages.hpp"

namespace aegis
{

using json = nlohmann::json;

AEGIS_DECL channel::channel(const snowflake channel_id, const snowflake guild_id, core * _bot, asio::io_context & _io, ratelimit::ratelimit_mgr & _ratelimit)
    : channel_id(channel_id)
    , guild_id(guild_id)
    , _guild(nullptr)
    , _io_context(_io)
    , _bot(_bot)
	, _ratelimit(_ratelimit)
{
}

AEGIS_DECL guild & channel::get_guild() const
{
    if (_guild == nullptr)
        throw exception("Guild not set", make_error_code(error::guild_not_found));
    return *_guild;
}

AEGIS_DECL guild & channel::get_guild(std::error_code & ec) const noexcept
{
    if (_guild == nullptr)
        ec = make_error_code(error::guild_not_found);
    else
        ec = error_code();
    return *_guild;
}

#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL permission channel::perms() const noexcept
{
    return permission(_guild->get_permissions(_guild->self(), this));
}

AEGIS_DECL void channel::_load_with_guild(guild & _guild, const json & obj, shards::shard * _shard)
{
    std::unique_lock<shared_mutex> l(_m);

    _load_with_guild_nolock(_guild, obj, _shard);
}

AEGIS_DECL void channel::_load_with_guild_nolock(guild & _guild, const json & obj, shards::shard * _shard)
{
    channel_id = obj["id"];
    guild_id = _guild.get_id();
    try
    {
        if (!obj["name"].is_null()) name = obj["name"].get<std::string>();
        position = obj["position"];
        type = static_cast<gateway::objects::channel::channel_type>(obj["type"].get<int>());// 0 = text, 2 = voice

        if (obj.count("nsfw"))
            _nsfw = obj["nsfw"];

        //voice channels
        if (obj.count("bitrate") && !obj["bitrate"].is_null())
        {
            bitrate = obj["bitrate"];
            user_limit = obj["user_limit"];
        }
        else
        {
            //not a voice channel, so has a topic field and last message id
            if (obj.count("topic") && !obj["topic"].is_null()) topic = obj["topic"].get<std::string>();
            if (obj.count("last_message_id") && !obj["last_message_id"].is_null()) last_message_id = obj["last_message_id"];
        }

        if (obj.count("parent_id") && !obj["parent_id"].is_null())
        {
            parent_id = obj["parent_id"];
        }

        if (obj.count("permission_overwrites") && !obj["permission_overwrites"].is_null())
        {
            json permission_overwrites = obj["permission_overwrites"];

            // check if override has been deleted
            auto newOverrides = overrides;
            for(auto& override : overrides) {
                bool deleted = true;
                for(auto permission : permission_overwrites) {
                    if(permission["id"] == override.second.id)
                        deleted = false;
                }
                if(deleted)
                    newOverrides.erase(override.first);
            }

            overrides = newOverrides;

            for (auto & permission : permission_overwrites)
            {
                uint32_t allow = permission["allow"];
                uint32_t deny = permission["deny"];
                snowflake p_id = permission["id"];
                std::string p_type = permission["type"];

                overrides[p_id].allow = allow;
                overrides[p_id].deny = deny;
                overrides[p_id].id = p_id;
                if (p_type == "role")
                    overrides[p_id].type = gateway::objects::overwrite_type::Role;
                else
                    overrides[p_id].type = gateway::objects::overwrite_type::User;
            }
        }

        //_channel.update_permission_cache();
    }
    catch (std::exception&e)
    {
        throw exception(fmt::format("Shard#{} : Error processing channel[{}] of guild[{}] {}", _shard->get_id(), channel_id, guild_id, e.what()), make_error_code(error::channel_error));
    }
}
#else
AEGIS_DECL void channel::_load_with_guild(guild & _guild, const json & obj, shards::shard * _shard)
{
    std::unique_lock<shared_mutex> l(_m);

    channel_id = obj["id"];
    guild_id = _guild.get_id();
}
#endif

AEGIS_DECL aegis::future<gateway::objects::message> channel::create_message(const std::string & content, int64_t nonce)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_send_messages())
        return aegis::make_exception_future<gateway::objects::message>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    obj["content"] = content;
    if (nonce)
        obj["nonce"] = nonce;

	std::string _endpoint = fmt::format("/channels/{}/messages", channel_id);
	return _ratelimit.post_task<gateway::objects::message>({ _endpoint, rest::Post, obj.dump(-1, ' ', true) });
}

AEGIS_DECL aegis::future<gateway::objects::message> channel::get_message(snowflake message_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_read_history())
        return aegis::make_exception_future<gateway::objects::message>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/messages/{}", channel_id, message_id);
    return _ratelimit.post_task<gateway::objects::message>({ _endpoint, rest::Get });
}

AEGIS_DECL aegis::future<gateway::objects::messages> channel::get_messages(get_messages_t obj)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_read_history())
        return aegis::make_exception_future<gateway::objects::messages>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string type_, query_params{ "?" }, limit;

    if (obj._limit >= 1 && obj._limit <= 100)
        limit = fmt::format("&limit={}", obj._limit);

    switch (obj._type)
    {
        case get_messages_t::get_messages_type::AFTER:
            query_params += fmt::format("after={}{}", obj._message_id, limit);
            break;
        case get_messages_t::get_messages_type::AROUND:
            query_params += fmt::format("after={}{}", obj._message_id, limit);
            break;
        case get_messages_t::get_messages_type::BEFORE:
            query_params += fmt::format("after={}{}", obj._message_id, limit);
            break;
        case get_messages_t::get_messages_type::LAST:
            query_params += limit.substr(1);
            break;
    }

    std::string _endpoint = fmt::format("/channels/{}/messages", channel_id);
    return _ratelimit.post_task<gateway::objects::messages>({ _endpoint, rest::Get, {}, {}, {}, {}, query_params });
}

AEGIS_DECL aegis::future<aegis::gateway::objects::message> channel::create_message(create_message_t obj)
{
    if (obj._file.has_value())
    {
#if !defined(AEGIS_DISABLE_ALL_CACHE)
        if (_guild && (!perms().can_send_messages() || !perms().can_attach_files()))
            return aegis::make_exception_future<gateway::objects::message>(error::no_permission);
#endif

        std::shared_lock<shared_mutex> l(_m);

        json jobj;
        if (!obj._content.empty())
            jobj["content"] = obj._content;
        if (!obj._embed.empty())
            jobj["embed"] = obj._embed;
        if (obj._nonce)
            jobj["nonce"] = obj._nonce;

        std::string _endpoint = fmt::format("/channels/{}/messages", channel_id);
        return _ratelimit.post_task<gateway::objects::message>({ _endpoint, rest::Post, jobj.dump(-1, ' ', true), {}, "443", {}, {}, obj._file });
    }
    else
        return create_message_embed(obj._content, obj._embed, obj._nonce);
};

AEGIS_DECL aegis::future<gateway::objects::message> channel::create_message_embed(const std::string & content, const json & embed, int64_t nonce)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_send_messages())
        return aegis::make_exception_future<gateway::objects::message>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    if (!content.empty())
        obj["content"] = content;
    if (!embed.empty())
        obj["embed"] = embed;
    if (nonce)
        obj["nonce"] = nonce;

	std::string _endpoint = fmt::format("/channels/{}/messages", channel_id);
	return _ratelimit.post_task<gateway::objects::message>({ _endpoint, rest::Post, obj.dump(-1, ' ', true) });
}

AEGIS_DECL aegis::future<gateway::objects::message> channel::create_message_embed(create_message_t obj)
{
    return create_message_embed(obj._content, obj._embed, obj._nonce);
}

AEGIS_DECL aegis::future<gateway::objects::message> channel::edit_message(edit_message_t obj)
{
    return edit_message_embed(obj);
}

AEGIS_DECL aegis::future<gateway::objects::message> channel::edit_message(snowflake message_id, const std::string & content)
{
    std::shared_lock<shared_mutex> l(_m);

    json obj;
    obj["content"] = content;

	std::string _endpoint = fmt::format("/channels/{}/messages/{}", channel_id, message_id);
	std::string _bucket = fmt::format("/channels/{}/messages/", channel_id);
	return _ratelimit.post_task<gateway::objects::message>(_bucket, { _endpoint, rest::Patch, obj.dump() });
}

AEGIS_DECL aegis::future<gateway::objects::message> channel::edit_message_embed(edit_message_t obj)
{
    return edit_message_embed(obj._message_id, obj._content, obj._embed);
}

AEGIS_DECL aegis::future<gateway::objects::message> channel::edit_message_embed(snowflake message_id, const std::string & content, const json & embed)
{
    std::shared_lock<shared_mutex> l(_m);

    json obj;
    if (content.empty() && embed.empty())
        return aegis::make_exception_future<gateway::objects::message>(error::bad_request);
    if (!content.empty())
        obj["content"] = content;
    if (!embed.empty())
        obj["embed"] = embed;

	std::string _endpoint = fmt::format("/channels/{}/messages/{}", channel_id, message_id);
	std::string _bucket = fmt::format("/channels/{}/messages/", channel_id);
	return _ratelimit.post_task<gateway::objects::message>(_bucket, { _endpoint, rest::Patch, obj.dump() });
}

/**
 * can delete your own messages freely - provide separate function or keep history of messages?
 * message::delete() can determine if author is bot for self-delete
 */
AEGIS_DECL aegis::future<rest::rest_reply> channel::delete_message(snowflake message_id)
{
    //@todo check if bot owns message before denying delete
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_manage_messages())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/messages/{}", channel_id, message_id);
	std::string _bucket = fmt::format("/channels/{}/messages/_/delete", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Delete });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::bulk_delete_message(const std::vector<int64_t> & messages)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_manage_messages())
        return aegis::make_exception_future(error::no_permission);
    if (messages.size() < 2 || messages.size() > 100)
        return aegis::make_exception_future(error::bulk_delete_out_of_range);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    json & msgs = obj["messages"];
    for (const auto & id : messages)
        msgs.push_back(std::to_string(id));

	std::string _endpoint = fmt::format("/channels/{}/messages/bulk-delete", channel_id);
	return _ratelimit.post_task({ _endpoint, rest::Post, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::bulk_delete_message(const std::vector<snowflake> & messages)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_manage_messages())
        return aegis::make_exception_future(error::no_permission);
    if (messages.size() < 2 || messages.size() > 100)
        return aegis::make_exception_future(error::bulk_delete_out_of_range);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    json & msgs = obj["messages"];
    for (const auto & id : messages)
        msgs.push_back(std::to_string(id));

	std::string _endpoint = fmt::format("/channels/{}/messages/bulk-delete", channel_id);
	return _ratelimit.post_task({ _endpoint, rest::Post, obj.dump() });
}

AEGIS_DECL aegis::future<gateway::objects::channel> channel::modify_channel(lib::optional<std::string> _name, lib::optional<int> _position, lib::optional<std::string> _topic,
                                    lib::optional<bool> _nsfw, lib::optional<int> _bitrate, lib::optional<int> _user_limit,
                                    lib::optional<std::vector<gateway::objects::permission_overwrite>> _permission_overwrites, lib::optional<snowflake> _parent_id,
                                    lib::optional<int> _rate_limit_per_user)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!_guild) return aegis::make_exception_future<gateway::objects::channel>(error::guild_error);
    if (!perms().can_manage_channels())
        return aegis::make_exception_future<gateway::objects::channel>(error::no_permission);
#endif

    json obj;
    if (_name.has_value())
        obj["name"] = _name.value();
    if (_position.has_value())
        obj["position"] = _position.value();
    if (_topic.has_value())
        obj["topic"] = _topic.value();
    if (_nsfw.has_value())
        obj["nsfw"] = _nsfw.value();
    if (_bitrate.has_value())//voice only
        obj["bitrate"] = _bitrate.value();
    if (_user_limit.has_value())//voice only
        obj["user_limit"] = _user_limit.value();
    if (_permission_overwrites.has_value())//requires OWNER
    {
// #if !defined(AEGIS_DISABLE_ALL_CACHE)
//         if (_guild->owner_id != _guild->self()->member_id)
//         {
//             ec = make_error_code(value::no_permission);
//             return {};
//         }
// #endif

        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : _permission_overwrites.value())
        {
            obj["permission_overwrites"].push_back(p_ow);
        }
    }
    if (_parent_id.has_value())//VIP only
        obj["parent_id"] = _parent_id.value();
    if (_rate_limit_per_user.has_value())
        obj["rate_limit_per_user"] = _rate_limit_per_user.value();

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}", channel_id);
	return _ratelimit.post_task<gateway::objects::channel>({ _endpoint, rest::Patch, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::delete_channel()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_manage_channels())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}", channel_id);
	return _ratelimit.post_task({ _endpoint, rest::Delete });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::create_reaction(snowflake message_id, const std::string & emoji_text)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_add_reactions())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text);
    std::string _bucket = fmt::format("/guilds/{}/reactions", guild_id);
	_ratelimit.get_bucket(_bucket).reset_bypass = 250;
    return _ratelimit.post_task(_bucket, { _endpoint, rest::Put });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::delete_own_reaction(snowflake message_id, const std::string & emoji_text)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_add_reactions())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text);
    std::string _bucket = fmt::format("/guilds/{}/reactions", guild_id);
	_ratelimit.get_bucket(_bucket).reset_bypass = 250;
    return _ratelimit.post_task(_bucket, { _endpoint, rest::Delete });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::delete_user_reaction(snowflake message_id, const std::string & emoji_text, snowflake member_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!_guild) return aegis::make_exception_future(error::guild_error);
    if (!perms().can_manage_messages())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/messages/{}/reactions/{}/{}", channel_id, message_id, emoji_text, member_id);
	std::string _bucket = fmt::format("/channels/{}/messages/_/reactions/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Delete });
}

/**\todo Support query parameters
 *  \todo before[snowflake], after[snowflake], limit[int]
 */
AEGIS_DECL aegis::future<rest::rest_reply> channel::get_reactions(snowflake message_id, const std::string & emoji_text)
{
    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/messages/{}/reactions/{}", channel_id, message_id, emoji_text);
	std::string _bucket = fmt::format("/channels/{}/messages/_/reactions/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Get });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::delete_all_reactions(snowflake message_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!_guild) return aegis::make_exception_future(error::guild_error);
    if (!perms().can_manage_messages())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/messages/{}/reactions", channel_id, message_id);
	std::string _bucket = fmt::format("/channels/{}/messages/_/reactions", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Delete });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::edit_channel_permissions(snowflake _overwrite_id, int64_t _allow, int64_t _deny, const std::string & _type)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!_guild) return aegis::make_exception_future(error::guild_error);
    if (!perms().can_manage_roles())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    obj["allow"] = _allow;
    obj["deny"] = _deny;
    obj["type"] = _type;
 
	std::string _endpoint = fmt::format("/channels/{}/permissions/{}", channel_id, _overwrite_id);
	std::string _bucket = fmt::format("/channels/{}/permissions/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Put, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::get_channel_invites()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!_guild) return aegis::make_exception_future(error::guild_error);
    if (!perms().can_manage_channels())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/invites", channel_id);
	return _ratelimit.post_task({ _endpoint, rest::Get });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::create_channel_invite(lib::optional<int> max_age, lib::optional<int> max_uses, lib::optional<bool> temporary, lib::optional<bool> unique)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!_guild) return aegis::make_exception_future(error::guild_error);
    if (!perms().can_invite())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    if (max_age.has_value())
        obj["max_age"] = max_age.value();
    if (max_uses.has_value())
        obj["max_uses"] = max_uses.value();
    if (temporary.has_value())
        obj["temporary"] = temporary.value();
    if (unique.has_value())
        obj["unique"] = unique.value();

	std::string _endpoint = fmt::format("/channels/{}/invites", channel_id);
	return _ratelimit.post_task({ _endpoint, rest::Post, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::delete_channel_permission(snowflake overwrite_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!_guild) return aegis::make_exception_future(error::guild_error);
    if (!perms().can_manage_roles())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/permissions/{}", channel_id, overwrite_id);
	std::string _bucket = fmt::format("/channels/{}/permissions/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Delete });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::trigger_typing_indicator()
{
    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/typing", channel_id);
	return _ratelimit.post_task({ _endpoint });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::get_pinned_messages()
{
    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/pins", channel_id);
	return _ratelimit.post_task({ _endpoint, rest::Get });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::add_pinned_channel_message(snowflake message_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_manage_messages())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/pins/{}", channel_id, message_id);
	std::string _bucket = fmt::format("/channels/{}/pins/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Put });
}

AEGIS_DECL aegis::future<rest::rest_reply> channel::delete_pinned_channel_message(snowflake message_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild && !perms().can_manage_messages())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/pins/{}", channel_id, message_id);
	std::string _bucket = fmt::format("/channels/{}/pins/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Delete });
}

/**\todo Will likely move to aegis class
 * requires OAuth permissions to perform
 */
AEGIS_DECL aegis::future<rest::rest_reply> channel::group_dm_add_recipient(snowflake user_id)//will go in aegis::aegis
{
    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/recipients/{}", channel_id, user_id);
	std::string _bucket = fmt::format("/channels/{}/recipients/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Put });
}

/**\todo Will likely move to aegis class
 * requires OAuth permissions to perform
 */
AEGIS_DECL aegis::future<rest::rest_reply> channel::group_dm_remove_recipient(snowflake user_id)//will go in aegis::aegis
{
    std::shared_lock<shared_mutex> l(_m);

    std::string _endpoint = fmt::format("/channels/{}/recipients/{}", channel_id, user_id);
	std::string _bucket = fmt::format("/channels/{}/recipients/", channel_id);
	return _ratelimit.post_task(_bucket, { _endpoint, rest::Delete });
}

}

