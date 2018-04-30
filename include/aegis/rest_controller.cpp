//
// rest_controller.cpp
// *******************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#include "aegis/rest_controller.hpp"
#if defined(AEGIS_PROFILING)
#include "aegis/core.hpp"
#endif
#include "aegis/rest_reply.hpp"
#include <string>
#include <asio/streambuf.hpp>
#include <asio/connect.hpp>

namespace aegis
{

namespace rest
{

#if defined(AEGIS_PROFILING)
AEGIS_DECL rest_controller::rest_controller(const std::string & _token, core * bot)
    : token{ _token }
    , prefix{ "/api/v6" }
    , bot(bot)
{

}
#else
AEGIS_DECL rest_controller::rest_controller(const std::string & _token)
    : token{ _token }
    , prefix{ "/api/v6" }
{

}
#endif

AEGIS_DECL rest_controller::~rest_controller()
{

}

AEGIS_DECL rest_reply rest_controller::get(const std::string & path)
{
    return call(path, "", "GET", "");
}

AEGIS_DECL rest_reply rest_controller::get(const std::string & path, const std::string & content, const std::string & host)
{
    return call(path, content, "GET", host);
}

AEGIS_DECL rest_reply rest_controller::post(const std::string & path, const std::string & content, const std::string & host)
{
    return call(path, content, "POST", host);
}

AEGIS_DECL rest_reply rest_controller::call(const std::string & path, const std::string & content, const std::string & method, const std::string & host)
{
    websocketpp::http::parser::response hresponse;

    int32_t limit = 0;
    int32_t remaining = 0;
    int64_t reset = 0;
    int32_t retry = 0;
    bool global = false;
    int32_t status_code = 0;

    try
    {
#ifdef AEGIS_PROFILING
        auto start_time = std::chrono::system_clock::now();
#endif
        asio::ip::basic_resolver<asio::ip::tcp>::results_type r;
        auto it = resolver_cache.find(host.empty() ? "discordapp.com" : host);
        if (it == resolver_cache.end())
        {
            asio::ip::tcp::resolver resolver(_io_context);
            r = resolver.resolve(host.empty() ? "discordapp.com" : host, "443");
        }
        else
            r = it->second;


        asio::ssl::context ctx(asio::ssl::context::tlsv12);

        ctx.set_options(
            asio::ssl::context::default_workarounds
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::no_sslv3);

        asio::ssl::stream<asio::ip::tcp::socket> socket(_io_context, ctx);
        SSL_set_tlsext_host_name(socket.native_handle(), (host.empty() ? "discordapp.com" : host.data()));

        asio::connect(socket.lowest_layer(), r);

        asio::error_code handshake_ec;
        socket.handshake(asio::ssl::stream_base::client, handshake_ec);

        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << method << " " << prefix << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << (host.empty() ? "discordapp.com" : host.data()) << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Authorization: Bot " << token << "\r\n";
        request_stream << "User-Agent: DiscordBot (https://github.com/zeroxs/aegis.cpp " << AEGIS_VERSION_LONG << ")\r\n";
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

        std::istream response_stream(&response);
        std::istringstream istrm(response_content.str());
        hresponse.consume(istrm);

        status_code = hresponse.get_status_code();

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

#ifdef AEGIS_PROFILING
        if (bot->call_end)
            bot->call_end(start_time);
#endif

        if (error != asio::error::eof && error != asio::ssl::error::stream_truncated)
            throw asio::system_error(error);
        return { rest_reply{ "", make_error_code(static_cast<aegis::error>(status_code)),
        hresponse.get_status_code(), global, limit, remaining, reset, retry, hresponse.get_body() } };
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";

        return { rest_reply{ e.what(),
            make_error_code(status_code ? static_cast<aegis::error>(status_code) : error::general),
        hresponse.get_status_code(), global, limit, remaining, reset, retry, hresponse.get_body() } };
    }
}

}

}
