//
// rest_controller.hpp
// *******************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/config.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <map>
#include <asio/ip/basic_resolver.hpp>
#include <asio/ip/tcp.hpp>

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

struct request_params
{
    std::string path;
    RequestMethod method = Post;
    std::string body;
    std::string host;
    std::vector<std::string> headers;
};

class rest_controller
{
public:
    AEGIS_DECL rest_controller(const std::string & token);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix, const std::string & host);
    AEGIS_DECL ~rest_controller();

    rest_controller(const rest_controller &) = delete;
    rest_controller(rest_controller &&) = delete;
    rest_controller & operator=(const rest_controller &) = delete;

    /// Performs an HTTP request on the path with content as the request body using the method method
    /**
     * @see rest_reply
     * @see rest::request_params
     * @param params A struct of HTTP parameters to perform the request
     * @returns rest_reply
     */
    AEGIS_DECL rest_reply execute(rest::request_params params);

    std::string get_method(RequestMethod method)
    {
        switch (method)
        {
            case Get:
                return "GET";
            case Post:
                return "POST";
            case Put:
                return "PUT";
            case Patch:
                return "PATCH";
            case Delete:
                return "DELETE";
            default:
                return "GET";
        }
    }

    void set_auth(const std::string & token)
    {
        _token = token;
    }

    void set_prefix(const std::string & prefix)
    {
        _prefix = prefix;
    }

private:
    std::string _token;
    std::string _prefix;
    std::string _host;
    std::unordered_map<std::string, asio::ip::basic_resolver<asio::ip::tcp>::results_type> _resolver_cache;
    asio::io_context _io_context;
};

}

}
