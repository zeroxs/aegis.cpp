//
// rest_controller.hpp
// *******************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"

#ifdef WIN32
# include "aegis/push.hpp"
#endif
#include <asio/ip/basic_resolver.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/connect.hpp>
#include <asio/streambuf.hpp>
#include <asio/ssl.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <websocketpp/http/request.hpp>
#include <websocketpp/http/parser.hpp>
#include <websocketpp/http/response.hpp>
#ifdef WIN32
# include "aegis/pop.hpp"
#endif

#include "aegis/rest/rest_reply.hpp"

#include <string>
#include <map>
#include <functional>

namespace aegis
{

namespace rest
{

enum RequestMethod
{
    Get,
    Post,
    Put,
    Patch,
    Delete,
    MAX_METHODS
};

struct aegis_file
{
    std::string name;
    std::vector<char> data;
};

struct request_params
{
    std::string path;
    RequestMethod method = Post;
    std::string body;
    std::string host;
    std::string port = "443";
    std::vector<std::string> headers;
    std::string _path_ex;
    lib::optional<aegis_file> file;
};

class rest_controller
{
public:
    AEGIS_DECL rest_controller(const std::string & token, asio::io_context * _io_context);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix, asio::io_context * _io_context);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix, const std::string & host, asio::io_context * _io_context);
    ~rest_controller() = default;

    rest_controller(const rest_controller &) = delete;
    rest_controller(rest_controller &&) = delete;
    rest_controller & operator=(const rest_controller &) = delete;

    /// Performs an HTTP request using the params provided
    /**
     * @see rest::rest_reply
     * @see rest::request_params
     * @param params A struct of HTTP parameters to perform the request
     * @returns rest::rest_reply
     * @throws asio::system_error
     */
    AEGIS_DECL rest_reply execute(rest::request_params && params);

    /// Performs an HTTP request using the params provided
    /**
     * @see rest::rest_reply
     * @see rest::request_params
     * @param params A struct of HTTP parameters to perform the request
     * @returns rest::rest_reply
     * @throws asio::system_error
     */
    AEGIS_DECL rest_reply execute2(rest::request_params && params);

    static std::string get_method(RequestMethod method) noexcept
    {
        switch (method)
        {
            case Post:
                return "POST";
            case Put:
                return "PUT";
            case Patch:
                return "PATCH";
            case Delete:
                return "DELETE";
            case Get:
            default:
                return "GET";
        }
    }

    void set_prefix(const std::string & prefix) noexcept
    {
        _prefix = prefix;
    }

    std::chrono::hours tz_bias()
    {
        return _tz_bias;
    }

    void tz_bias(std::chrono::hours bias)
    {
        _tz_bias = bias;
    }

private:
    friend aegis::core;
    std::string _token;
    std::string _prefix;
    std::string _host;
    std::unordered_map<std::string, asio::ip::basic_resolver<asio::ip::tcp>::results_type> _resolver_cache;

    using rest_end_t = std::function<void(std::chrono::steady_clock::time_point, uint16_t)>;
    rest_end_t rest_end;
    asio::io_context * _io_context = nullptr;
    std::chrono::hours _tz_bias = 0h;
};

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/rest/impl/rest_controller.cpp"
#endif
