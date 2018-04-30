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
#include "aegis/snowflake.hpp"
#include "aegis/error.hpp"
#include "aegis/rest_reply.hpp"

#include <vector>
#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/sink.h"

#ifdef WIN32
# include "aegis/push.hpp"
# include "websocketpp/config/asio_client.hpp"
# include "websocketpp/client.hpp"
# include "aegis/pop.hpp"
#endif

#ifdef REDIS
# include "aegis/push.hpp"
# ifdef WIN32
#  include <redisclient/redissyncclient.h>
# else
#  include <redisclient/redissyncclient.h>
# endif
# include "aegis/pop.hpp"
#else
# include <asio.hpp>
#endif

#include <asio/bind_executor.hpp>

namespace aegis
{

class core;

namespace rest
{

/// Primary class for managing REST interactions
class rest_controller
{
public:
    /**
     * @see shard
     * @see guild
     * @see channel
     * @see member
     *
     * @param token A string of the authentication token
     */
#if !defined(AEGIS_PROFILING)
    AEGIS_DECL explicit rest_controller(const std::string & _token);
#else
    AEGIS_DECL explicit rest_controller(const std::string & _token, core * bot);
#endif

    /// Destroys the shards, stops the asio::work object, destroys the websocket object, and attempts to join the rest_thread thread
    ///
    AEGIS_DECL ~rest_controller();

    rest_controller(const rest_controller &) = delete;
    rest_controller(rest_controller &&) = delete;
    rest_controller & operator=(const rest_controller &) = delete;

    std::mutex m;
    std::condition_variable cv;
    asio::io_context _io_context;

    /// Performs a GET request on the path
    /**
     * @see rest_reply
     * @param path A string of the uri path to get
     *
     * @param host Provide only if the call should go to a different host
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
    AEGIS_DECL rest_reply call(const std::string & path, const std::string & content, const std::string & method, const std::string & host = "");

    void set_prefix(const std::string p)
    {
        prefix = p;
    }

private:
    // Bot's token
    const std::string & token;
    std::string prefix;
    std::map<std::string, asio::ip::basic_resolver<asio::ip::tcp>::results_type> resolver_cache;
#if defined(AEGIS_PROFILING)
    core * bot;
#endif
};

}

}
