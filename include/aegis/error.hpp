//
// error.hpp
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
#include <exception>
#include <string>
#include <utility>
#include <system_error>


namespace aegiscpp
{
using err_str_pair = std::pair<std::error_code, std::string>;

using error_code = std::error_code;

/**\todo Needs documentation
*/
namespace error
{

/**\todo Needs documentation
*/
enum value
{
    /// Catch-all library error
    general = 1,

    /// Token invalid
    invalid_token,

    /// Bot was in the wrong state for this operation
    invalid_state,

    /// REST did not return websocket gateway url
    get_gateway,

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

    std::string message(int value) const override
    {
        switch (value)
        {
            case error::general:
                return "Generic error";
            case error::invalid_token:
                return "Token invalid";
            case error::invalid_state:
                return "Invalid state";
            case error::get_gateway:
                return "Unable to retrieve Gateway data";
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
                return "Rate Limited";
            case error::global_rate_limited:
                return "Globally Rate Limited";
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
inline std::error_code make_error_code(error::value e)
{
    return std::error_code(static_cast<int>(e), get_category());
}

}
}

namespace aegiscpp
{

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
