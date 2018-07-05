//
// rest_controller.cpp
// *******************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#include "aegis/rest/rest_controller.hpp"
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "aegis/core.hpp"

namespace aegis
{

namespace rest
{

#if defined(AEGIS_PROFILING)
AEGIS_DECL rest_controller::rest_controller(const std::string & token, core * bot)
    : _token(token)
    , _bot(bot)
{

}

AEGIS_DECL rest_controller::rest_controller(const std::string & token, const std::string & prefix, core * bot)
    : _token(token)
    , _prefix(prefix)
    , _bot(bot)
{

}

AEGIS_DECL rest_controller::rest_controller(const std::string & token, const std::string & prefix, const std::string & host, core * bot)
    : _token(token)
    , _prefix(prefix)
    , _host(host)
    , _bot(bot)
{

}
#else
AEGIS_DECL rest_controller::rest_controller(const std::string & token)
    : _token(token)
{

}

AEGIS_DECL rest_controller::rest_controller(const std::string & token, const std::string & prefix)
    : _token(token)
    , _prefix(prefix)
{

}

AEGIS_DECL rest_controller::rest_controller(const std::string & token, const std::string & prefix, const std::string & host)
    : _token(token)
    , _prefix(prefix)
    , _host(host)
{

}
#endif

AEGIS_DECL rest_controller::~rest_controller()
{
    _io_context.stop();
}

AEGIS_DECL rest_reply rest_controller::get(const std::string & path)
{
    return execute(path, "", "GET", "");
}

AEGIS_DECL rest_reply rest_controller::get(const std::string & path, const std::string & content, const std::string & host)
{
    return execute(path, content, "GET", host);
}

AEGIS_DECL rest_reply rest_controller::post(const std::string & path)
{
    return execute(path, "", "POST", "");
}

AEGIS_DECL rest_reply rest_controller::post(const std::string & path, const std::string & content, const std::string & host)
{
    return execute(path, content, "POST", host);
}

AEGIS_DECL rest_reply rest_controller::execute(const std::string & path, const std::string & content, const std::string & method, const std::string & host)
{
    if (_host.empty() && host.empty())
        throw aegis::exception("REST host not set");

    websocketpp::http::parser::response hresponse;

    int32_t limit = 0;
    int32_t remaining = 0;
    int64_t reset = 0;
    int32_t retry = 0;
    bool global = false;

    try
    {
#if defined(AEGIS_PROFILING)
        auto start_time = std::chrono::steady_clock::now();
#endif

        asio::ip::basic_resolver<asio::ip::tcp>::results_type r;

        const std::string & tar_host = host.empty() ? _host : host;

        //TODO: make cache expire?
        auto it = _resolver_cache.find(tar_host);
        if (it == _resolver_cache.end())
        {
            asio::ip::tcp::resolver resolver(_io_context);
            r = resolver.resolve(tar_host, "443");
            _resolver_cache.emplace(tar_host, r);
        }
        else
            r = it->second;

        asio::ssl::context ctx(asio::ssl::context::tlsv12);

        ctx.set_options(
            asio::ssl::context::default_workarounds
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::no_sslv3);

        asio::ssl::stream<asio::ip::tcp::socket> socket(_io_context, ctx);
        SSL_set_tlsext_host_name(socket.native_handle(), tar_host.data());

        asio::connect(socket.lowest_layer(), r);

        asio::error_code handshake_ec;
        socket.handshake(asio::ssl::stream_base::client, handshake_ec);

        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << method << " " << _prefix << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << tar_host << "\r\n";
        request_stream << "Accept: */*\r\n";
        if (tar_host == "discordapp.com")
            request_stream << "Authorization: Bot " << _token << "\r\n";
        else
            request_stream << "Authorization: " << _token << "\r\n";
        request_stream << "User-Agent: DiscordBot (https://github.com/zeroxs/aegis.cpp, " << AEGIS_VERSION_LONG << ")\r\n";
        request_stream << "Content-Length: " << content.size() << "\r\n";
        request_stream << "Content-Type: application/json\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream << content;

        asio::write(socket, request);
        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");
        std::stringstream response_content;
        response_content << &response;

        asio::error_code error;
        while (asio::read(socket, response, asio::transfer_at_least(1), error))
            response_content << &response;

        std::istringstream istrm(response_content.str());
        hresponse.consume(istrm);

        auto test = hresponse.get_header("X-RateLimit-Limit");
        if (!test.empty())
            limit = std::stoul(test);
        test = hresponse.get_header("X-RateLimit-Remaining");
        if (!test.empty())
            remaining = std::stoul(test);
        test = hresponse.get_header("X-RateLimit-Reset");
        if (!test.empty())
            reset = std::stoul(test);
        test = hresponse.get_header("Retry-After");
        if (!test.empty())
            retry = std::stoul(test);

        global = !(hresponse.get_header("X-RateLimit-Global").empty());

#if defined(AEGIS_PROFILING)
        if (_bot->call_end)
            _bot->call_end(start_time);
#endif

        if (error != asio::error::eof && error != asio::ssl::error::stream_truncated)
            throw asio::system_error(error);
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return rest_reply(static_cast<http_code>(hresponse.get_status_code()),
                        global, limit, remaining, reset, retry, hresponse.get_body());
}

}

}
