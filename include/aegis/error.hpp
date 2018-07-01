//
// error.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <exception>
#include <string>
#include <utility>
#include <system_error>

namespace aegis
{
using err_str_pair = std::pair<std::error_code, std::string>;

using error_code = std::error_code;

/**\todo Needs documentation
 */
enum error
{
    /// Catch-all library error
    general = 1,

    /// Token invalid
    invalid_token,

    /// Bot was in the wrong state for this operation
    invalid_state,

    /// REST did not return websocket gateway url
    get_gateway,

    /// No gateway url set
    no_gateway,

    /// Does not have permission to perform this action
    no_permission,

    /// Feature not yet implemented
    not_implemented,

    /// Member not found
    member_not_found,

    /// Channel not found
    channel_not_found,

    /// Guild not found
    guild_not_found,

    /// Rate Limited
    rate_limited,

    /// Globally Rate Limited
    global_rate_limited,

    /// Member related error
    member_error,

    /// Channel related error
    channel_error,

    /// Guild related error
    guild_error,

    /// Shard related error
    shard_error,

    /// Malformed Redis request
    bad_redis_request,

    max_errors
};

/**\todo Needs documentation
 */
class category : public std::error_category
{
public:
    category() = default;

    char const * name() const noexcept override
    {
        return "aegis";
    }

    std::string message(int val) const override
    {
        switch (val)
        {
            case error::general:
                return "Generic error";
            case error::invalid_token:
                return "Token invalid";
            case error::invalid_state:
                return "Invalid state";
            case error::get_gateway:
                return "Unable to retrieve gateway data";
            case error::no_gateway:
                return "No gateway url set";
            case error::no_permission:
                return "No permission for this action";
            case error::not_implemented:
                return "Feature not yet implemented";
            case error::member_not_found:
                return "Member not found";
            case error::channel_not_found:
                return "Channel not found";
            case error::guild_not_found:
                return "Guild not found";
            case error::rate_limited:
                return "Rate limited";
            case error::global_rate_limited:
                return "Globally rate limited";
            case error::member_error:
                return "Member related error";
            case error::channel_error:
                return "Channel related error";
            case error::guild_error:
                return "Guild related error";
            case error::shard_error:
                return "Shard related error";
            case error::bad_redis_request:
                return "Bad Redis request";
            default:
                return "Unknown";
        }
    }
};

/**\todo Needs documentation
 */
inline const std::error_category & get_category()
{
    static category instance;
    return instance;
}

/**\todo Needs documentation
 */
inline std::error_code make_error_code(error e)
{
    return std::error_code(static_cast<int>(e), get_category());
}

/**\todo Needs documentation
 */
class exception : public std::exception
{
public:
    exception(std::string const & msg, std::error_code ec = make_error_code(error::general))
        : _msg(msg.empty() ? ec.message() : msg)
        , _code(ec)
    {
    }

    explicit exception(std::error_code ec)
        : _msg(ec.message())
        , _code(ec)
    {
    }

    ~exception() = default;

    virtual char const * what() const noexcept override
    {
        return _msg.c_str();
    }

    std::error_code code() const noexcept
    {
        return _code;
    }

    const std::string _msg;
    std::error_code _code;
};

}

namespace std
{

template<> struct is_error_code_enum<aegis::error>
{
    static const bool value = true;
};

}
