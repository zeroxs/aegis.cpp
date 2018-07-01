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

class rest_controller
{
public:
#if defined(AEGIS_PROFILING)
    AEGIS_DECL rest_controller(const std::string & token, core * bot);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix, core * bot);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix, const std::string & host, core * bot);
#else
    AEGIS_DECL rest_controller(const std::string & token);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix);
    AEGIS_DECL rest_controller(const std::string & token, const std::string & prefix, const std::string & host);
#endif
    AEGIS_DECL ~rest_controller();

    rest_controller(const rest_controller &) = delete;
    rest_controller(rest_controller &&) = delete;
    rest_controller & operator=(const rest_controller &) = delete;

    /// Performs a GET request on the path
    /**
     * @see rest_reply
     * @param path A string of the uri path to get
     *
     * @returns Response object
     */
    AEGIS_DECL rest_reply get(const std::string & path);

    /// Performs a GET request on the path with content as the request body
    /**
    * @see rest_reply
    * @param path A string of the uri path to get
    *
    * @param content JSON formatted string to send as the body
    *
    * @param host Provide only if the call should go to a different host
    *
    * @returns Response object
    */
    AEGIS_DECL rest_reply get(const std::string & path, const std::string & content, const std::string & host = "");

    /// Performs a POST request on the path
    /**
    * @see rest_reply
    * @param path A string of the uri path to post
    *
    * @returns Response object
    */
    AEGIS_DECL rest_reply post(const std::string & path);

    /// Performs a POST request on the path with content as the request body
    /**
    * @see rest_reply
    * @param path A string of the uri path to post
    *
    * @param content JSON formatted string to send as the body
    *
    * @param host Provide only if the call should go to a different host
    *
    * @returns Response object
    */
    AEGIS_DECL rest_reply post(const std::string & path, const std::string & content, const std::string & host = "");

    /// Performs an HTTP request on the path with content as the request body using the method method
    /**
    * @see rest_reply
    * @param path A string of the uri path to get
    *
    * @param content JSON formatted string to send as the body
    *
    * @param method The HTTP method of the request
    *
    * @param host Provide only if the call should go to a different host
    *
    * @returns Response object
    */
    AEGIS_DECL rest_reply execute(const std::string & path, const std::string & content, const std::string & method, const std::string & host = "");

    AEGIS_DECL void set_auth(const std::string & token)
    {
        _token = token;
    }

    AEGIS_DECL void set_prefix(const std::string & prefix)
    {
        _prefix = prefix;
    }

private:
    std::string _token;
    std::string _prefix;
    std::string _host;
    std::map<std::string, asio::ip::basic_resolver<asio::ip::tcp>::results_type> _resolver_cache;
    asio::io_context _io_context;
#if defined(AEGIS_PROFILING)
    core * _bot;
#endif
};

}

}
