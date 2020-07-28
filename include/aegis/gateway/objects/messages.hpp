//
// messages.hpp
// ************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/shards/shard.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "aegis/gateway/objects/message.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

class message;

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, objects::messages& m);

inline void to_json(nlohmann::json& j, const objects::messages& m);
/// \endcond

/**\todo Needs documentation
 */
class messages
{
public:

    /// Constructor for the messages array
    /**
     * @param _core Pointer of core object
     */
    messages(aegis::core * _core) noexcept
        : _core(_core)
    {
    }

    /// Constructor for the message object
    /**
     * @param _json JSON string of the message object
     * @param bot Pointer of core object
     */
    messages(const std::string& _json, aegis::core* _core) noexcept
        : _core(_core)
    {
        from_json(nlohmann::json::parse(_json), *this);
    }

    /// Constructor for the messages object
    /**
     * @param _json JSON object of the messages array
     * @param _core Pointer of core object
     */
    messages(const nlohmann::json & _json, aegis::core * _core) noexcept
        : _core(_core)
    {
        from_json(_json, *this);
    }

    /// Set the guild of the message object. This is mostly an internal function
    /// though is left public for lower level use
    /**
     * @param _guild Pointer of the guild to assign the message to
     */
    AEGIS_DECL void set_guild(aegis::guild * _guild)
    {
        this->_guild = _guild;
    }

    messages() = default;
    messages& operator=(const messages&) = default;
    messages(const messages&) = default;
    messages(messages&& msg) = default;

    std::vector<aegis::gateway::objects::message>::iterator begin()
    {
        return _messages.begin();
    }

    std::vector<aegis::gateway::objects::message>::iterator end()
    {
        return _messages.end();
    }

    std::vector<aegis::gateway::objects::message>::reverse_iterator rbegin()
    {
        return _messages.rbegin();
    }

    std::vector<aegis::gateway::objects::message>::reverse_iterator rend()
    {
        return _messages.rend();
    }

    std::vector<aegis::gateway::objects::message> _messages; /**< array of messages */

private:
    friend inline void from_json(const nlohmann::json& j, objects::messages& m);
    friend inline void to_json(nlohmann::json& j, const objects::messages& m);
    friend class aegis::core;

    std::string _content;/**< String of the message contents */
    aegis::channel * _channel = nullptr;/**< Pointer to the channel this message belongs to */
    aegis::guild * _guild = nullptr;/**< Pointer to the guild this message belongs to */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    aegis::user * _user = nullptr;/**< Pointer to the author of this message */
#endif
    aegis::core * _core = nullptr;
    snowflake _message_id = 0; /**< snowflake of the message */
    snowflake _channel_id = 0; /**< snowflake of the channel this message belongs to */
    snowflake _guild_id = 0; /**< snowflake of the guild this message belongs to */
    snowflake _author_id = 0; /**< snowflake of the author of this message */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, objects::messages& m)
{
    if (j.size())
        for (const auto& _message : j)
            m._messages.push_back(_message);
}

inline void to_json(nlohmann::json& j, const objects::messages& m)
{
    if (!m._messages.empty())
        for (const auto& _message : m._messages)
            j.push_back(_message);
}
/// \endcond

}

}

}
