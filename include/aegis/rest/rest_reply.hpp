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
#include "aegis/utility.hpp"
#include "aegis/error.hpp"
#include <stdint.h>
#include <string>
#include <chrono>

namespace aegis
{

namespace rest
{

using namespace std::chrono_literals;

enum http_code
{
    unknown = 0,

    /// HTTP error codes for exceptions
    continue_code = 100,
    switching_protocols = 101,

    ok = 200,
    created = 201,
    accepted = 202,
    non_authoritative_information = 203,
    no_content = 204,
    reset_content = 205,
    partial_content = 206,

    multiple_choices = 300,
    moved_permanently = 301,
    found = 302,
    see_other = 303,
    not_modified = 304,
    use_proxy = 305,
    temporary_redirect = 307,

    bad_request = 400,
    unauthorized = 401,
    payment_required = 402,
    forbidden = 403,
    not_found = 404,
    method_not_allowed = 405,
    not_acceptable = 406,
    proxy_authentication_required = 407,
    request_timeout = 408,
    conflict = 409,
    gone = 410,
    length_required = 411,
    precondition_failed = 412,
    request_entity_too_large = 413,
    request_uri_too_long = 414,
    unsupported_media_type = 415,
    request_range_not_satisfiable = 416,
    expectation_failed = 417,
    im_a_teapot = 418,
    upgrade_required = 426,
    precondition_required = 428,
    too_many_requests = 429,
    request_header_fields_too_large = 431,

    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503,
    gateway_timeout = 504,
    http_version_not_supported = 505,
    not_extended = 510,
    network_authentication_required = 511,

    /// Cloudflare specific
    unknown_error = 520,
    server_down = 521,
    timed_out = 522,
    origin_unreachable = 523,
    timeout_occurred = 524,
    ssl_handshake_failed = 525,
    invalid_ssl_certificate = 526,
    railgun_error = 527,

    max_codes
};

/// REST responses with error_code for possible exception throwing
class rest_reply
{
public:
    explicit rest_reply(std::string const & msg)
        : _msg{ msg }
        , reply_code(http_code::unknown)
    {
    }

    rest_reply()
        : _msg("")
        , reply_code(http_code::unknown)
    {

    }

    rest_reply(http_code reply_code, bool global, int32_t limit, int32_t remaining, int64_t reset, int32_t retry, const std::string & content, std::chrono::steady_clock::duration exec_time = 0ms)
        : reply_code(reply_code)
        , global(global)
        , limit(limit)
        , remaining(remaining)
        , reset(reset)
        , retry(retry)
        , content(content)
        , execution_time(exec_time)
    {
    }

    rest_reply(const std::string & msg, http_code reply_code = http_code::unknown, bool global = false, int32_t limit = 0, int32_t remaining = 0, int64_t reset = 0, int32_t retry = 0, const std::string & content = "", std::chrono::steady_clock::duration exec_time = 0ms)
        : _msg(msg)
        , reply_code(reply_code)
        , global(global)
        , limit(limit)
        , remaining(remaining)
        , reset(reset)
        , retry(retry)
        , content(content)
        , execution_time(exec_time)
    {
    }

    operator bool()
    {
        if (reply_code == http_code::ok || reply_code == http_code::created || reply_code == http_code::accepted || reply_code == http_code::no_content)
            return true;
        return false;
    }

    ~rest_reply() = default;

private:
    std::string _msg;

public:
    http_code reply_code; /**< REST HTTP reply code */
    bool global = false; /**< Is global ratelimited */
    int32_t limit = 0; /**< Rate limit current endpoint call limit */
    int32_t remaining = 0; /**< Rate limit remaining count */
    int64_t reset = 0; /**< Rate limit reset time */
    int32_t retry = 0; /**< Rate limit retry time */
    std::string content; /**< REST call's reply body */
    bool permissions = true; /**< Whether the call had proper permissions */
    std::chrono::steady_clock::duration execution_time;
};

}

}
