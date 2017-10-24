//
// channel.hpp
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
#include "objects/permission_overwrite.hpp"


namespace aegiscpp
{

using json = nlohmann::json;
using rest_limits::bucket_factory;

class guild;
class shard;

class channel
{
public:
    explicit channel(snowflake channel_id, snowflake guild_id, bucket_factory & ratelimit, bucket_factory & emoji)
        : channel_id(channel_id)
        , guild_id(guild_id)
        , ratelimit(ratelimit)
        , emoji(emoji)
        , log(spdlog::get("aegis"))
        , _guild(0)
    {

    }

    snowflake channel_id;
    snowflake guild_id;
    bucket_factory & ratelimit;
    bucket_factory & emoji;
    std::shared_ptr<spdlog::logger> log;

    guild * _guild;

    snowflake last_message_id = 0;
    std::string name;
    std::string topic;
    uint32_t position = 0;
    channel_type type = channel_type::Text;

    uint16_t bitrate = 0;
    uint16_t user_limit = 0;

    std::unordered_map<int64_t, permission_overwrite> overrides;

    guild & get_guild();


    void load_with_guild(guild & _guild, const json & obj, shard * _shard);

    bool create_message(std::string content, std::function<void(rest_reply)> callback = nullptr);

    bool create_message_embed(std::string content, const json embed, std::function<void(rest_reply)> callback = nullptr);

    bool edit_message(snowflake message_id, std::string content, std::function<void(rest_reply)> callback = nullptr);

    bool edit_message_embed(snowflake message_id, std::string content, json embed, std::function<void(rest_reply)> callback = nullptr);

    bool delete_message(snowflake message_id, std::function<void(rest_reply)> callback = nullptr);

    bool bulk_delete_message(snowflake message_id, std::vector<int64_t> messages, std::function<void(rest_reply)> callback = nullptr);

    bool modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                        std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                        std::optional<std::vector<permission_overwrite>> permission_overwrites, std::optional<snowflake> parent_id, std::function<void(rest_reply)> callback = nullptr);

    bool delete_channel(std::function<void(rest_reply)> callback = nullptr);

    bool create_reaction(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback = nullptr);

    bool delete_own_reaction(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback = nullptr);

    bool delete_user_reaction(snowflake message_id, std::string emoji_text, snowflake user_id, std::function<void(rest_reply)> callback = nullptr);

    bool get_reactions(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback = nullptr);

    bool delete_all_reactions(snowflake message_id, std::function<void(rest_reply)> callback = nullptr);

    bool edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type, std::function<void(rest_reply)> callback = nullptr);

    bool get_channel_invites(std::function<void(rest_reply)> callback = nullptr);

    bool create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique, std::function<void(rest_reply)> callback = nullptr);

    bool delete_channel_permission(snowflake overwrite_id, std::function<void(rest_reply)> callback = nullptr);

    bool trigger_typing_indicator(std::function<void(rest_reply)> callback = nullptr);

    bool get_pinned_messages(std::function<void(rest_reply)> callback = nullptr);

    bool add_pinned_channel_message(std::function<void(rest_reply)> callback = nullptr);

    bool delete_pinned_channel_message(std::function<void(rest_reply)> callback = nullptr);

    bool group_dm_add_recipient(std::function<void(rest_reply)> callback = nullptr);//will go in aegis::aegis_core

    bool group_dm_remove_recipient(std::function<void(rest_reply)> callback = nullptr);//will go in aegis::aegis_core

};

}

