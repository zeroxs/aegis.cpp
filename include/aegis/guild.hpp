//
// guild.hpp
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
#include "channel.hpp"
#include "member.hpp"
#include "state_c.hpp"


namespace aegiscpp
{

using rest_limits::bucket_factory;
using json = nlohmann::json;

class channel;
class member;

class guild
{
public:
    explicit guild(int16_t shard_id, bot_state * state, snowflake id, bucket_factory & ratelimit)
        : shard_id(shard_id)
        , guild_id(id)
        , ratelimit(ratelimit)
        , log(spdlog::get("aegis"))
        , state(state)
    {
        unavailable = false;
    }

    ~guild();

    guild(const guild &) = delete;
    guild(guild &&) = delete;
    guild & operator=(const guild &) = delete;

    int16_t shard_id;
    snowflake guild_id;
    bucket_factory & ratelimit;
    std::shared_ptr<spdlog::logger> log;
    bot_state * state;

    member * self() const
    {
        auto self_id = state->user.id;
        auto slf = get_member(self_id);
        if (slf == nullptr)
            throw std::runtime_error("guild::self() is nullptr");
        return slf;
    }

    std::string get_name()
    {
        return name;
    }

    channel * get_channel_create(snowflake id, shard * shard) noexcept;

    void load(const json & obj, shard * shard) noexcept;
    
    void load_presence(const json & obj) noexcept;
    
    void load_role(const json & obj) noexcept;

    channel * get_channel(snowflake id) const noexcept;

    member * get_member(snowflake member_id) const noexcept;

    int32_t get_member_count() const noexcept;

    permission get_permissions(snowflake guild_id, snowflake channel_id) noexcept;

    permission get_permissions(member & _member, channel & _channel) noexcept;

    int64_t base_permissions(member * _member) const noexcept
    {
        return base_permissions(*_member);
    }

    int64_t base_permissions(member & _member) const noexcept;

    //permission compute_overwrites
    int64_t compute_overwrites(int64_t _base_permissions, member & _member, channel & _channel) const noexcept;

    role & get_role(int64_t r) const;

    void remove_member(const json & obj);

    void remove_role(snowflake role_id);



    /// REST CALLS

    // move this to aegis::aegis?
    bool create_guild(std::function<void(rest_reply)> callback = nullptr);

    bool get_guild(std::function<void(rest_reply)> callback = nullptr);

    bool modify_guild(std::optional<std::string> name, std::optional<std::string> voice_region, std::optional<int> verification_level,
                      std::optional<int> default_message_notifications, std::optional<snowflake> afk_channel_id, std::optional<int> afk_timeout,
                      std::optional<std::string> icon, std::optional<snowflake> owner_id, std::optional<std::string> splash, std::function<void(rest_reply)> callback = nullptr);

    bool delete_guild(std::function<void(rest_reply)> callback = nullptr);

    bool create_text_channel(std::string name, int64_t parent_id, bool nsfw, std::vector<permission_overwrite> permission_overwrites, std::function<void(rest_reply)> callback = nullptr);

    bool create_voice_channel(std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id, bool nsfw, std::vector<permission_overwrite> permission_overwrites, std::function<void(rest_reply)> callback = nullptr);
    
    bool create_category_channel(std::string name, int64_t parent_id, std::vector<permission_overwrite> permission_overwrites, std::function<void(rest_reply)> callback = nullptr);

    bool modify_channel_positions();

    bool modify_guild_member(snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                             std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles, std::optional<snowflake> channel_id, std::function<void(rest_reply)> callback = nullptr);

    bool modify_my_nick(std::string newname, std::function<void(rest_reply)> callback = nullptr);

    bool add_guild_member_role(snowflake user_id, snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    bool remove_guild_member_role(snowflake user_id, snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    bool remove_guild_member(snowflake user_id, std::function<void(rest_reply)> callback = nullptr);

    bool create_guild_ban(snowflake user_id, int8_t delete_message_days, std::function<void(rest_reply)> callback = nullptr);

    bool remove_guild_ban(snowflake user_id, std::function<void(rest_reply)> callback = nullptr);

    bool create_guild_role(std::function<void(rest_reply)> callback = nullptr);

    bool modify_guild_role_positions(std::function<void(rest_reply)> callback = nullptr);

    bool modify_guild_role(snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    bool delete_guild_role(snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    bool get_guild_prune_count(int16_t days, std::function<void(rest_reply)> callback = nullptr);

    bool begin_guild_prune(int16_t days, std::function<void(rest_reply)> callback = nullptr);

    bool get_guild_invites(std::function<void(rest_reply)> callback = nullptr);

    bool get_guild_integrations(std::function<void(rest_reply)> callback = nullptr);

    bool create_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    bool modify_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    bool delete_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    bool sync_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    bool get_guild_embed(std::function<void(rest_reply)> callback = nullptr);

    bool modify_guild_embed(std::function<void(rest_reply)> callback = nullptr);

    bool leave(std::function<void(rest_reply)> callback = nullptr);

    std::unordered_map<int64_t, std::shared_ptr<channel>> channels;
    std::unordered_map<int64_t, std::shared_ptr<member>> members;
    std::unordered_map<int64_t, std::unique_ptr<role>> roles;

private:
    friend class aegis;
    friend class channel;
    friend class member;

    std::string name;
    std::string m_icon;
    std::string m_splash;
    snowflake m_owner_id = 0;
    std::string m_region;
    snowflake m_afk_channel_id = 0;
    uint32_t m_afk_timeout = 0;//in seconds
    bool m_embed_enabled = false;
    snowflake m_embed_channel_id = 0;
    uint32_t m_verification_level = 0;
    uint32_t m_default_message_notifications = 0;
    uint32_t m_mfa_level = 0;
    std::string joined_at;
    bool m_large = false;
    bool unavailable = false;
    uint32_t m_member_count = 0;
    //std::string m_voice_states;//this is really an array

    bool m_silenterrors = false;
    bool m_silentperms = false;
    bool m_preventbotparse = false;
};

}
