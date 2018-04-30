//
// rest_reply.hpp
// **************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/config.hpp"
#include "aegis/error.hpp"
#include <stdint.h>
#include <string>
#include "aegis/push.hpp"
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "aegis/pop.hpp"

namespace aegis
{

namespace rest
{

/// REST responses with error_code for possible exception throwing
class rest_reply : public std::exception
{
public:
    rest_reply(std::string const & msg, std::error_code ec = make_error_code(error::general))
        : _msg(msg.empty() ? ec.message() : msg)
        , _code(ec)
    {
    }

    explicit rest_reply(std::error_code ec)
        : _msg(ec.message())
        , _code(ec)
    {
    }

    rest_reply()
        : _msg("")
    {

    }

    rest_reply(websocketpp::http::status_code::value reply_code, bool global, int32_t limit, int32_t remaining, int64_t reset, int32_t retry, std::string content)
        : reply_code(reply_code)
        , global(global)
        , limit(limit)
        , remaining(remaining)
        , reset(reset)
        , retry(retry)
        , content(content)
    {
    }

    rest_reply(std::string const & msg, std::error_code ec = make_error_code(error::general), websocketpp::http::status_code::value reply_code = websocketpp::http::status_code::value::uninitialized, bool global = false , int32_t limit = 0, int32_t remaining = 0, int64_t reset = 0, int32_t retry = 0, std::string content = "")
        : _msg(msg.empty() ? ec.message() : msg)
        , _code(ec)
        , reply_code(reply_code)
        , global(global)
        , limit(limit)
        , remaining(remaining)
        , reset(reset)
        , retry(retry)
        , content(content)
    {
    }

    operator bool()
    {
        if (reply_code == 200 || reply_code == 201 || reply_code == 202 || reply_code == 204)
            return true;
        return false;
    }

    ~rest_reply() = default;

    virtual char const * what() const noexcept override
    {
        return _msg.c_str();
    }

    std::error_code code() const noexcept
    {
        return _code;
    }

private:
    std::string _msg;
    std::error_code _code;

public:
    websocketpp::http::status_code::value reply_code; /**< REST HTTP reply code */
    bool global = false; /**< Is global ratelimited */
    int32_t limit = 0; /**< Rate limit current endpoint call limit */
    int32_t remaining = 0; /**< Rate limit remaining count */
    int64_t reset = 0; /**< Rate limit reset time */
    int32_t retry = 0; /**< Rate limit retry time */
    std::string content; /**< REST call's reply body */
    bool permissions = true; /**< Whether the call had proper permissions */
};

}

}
