//
// rest_controller.cpp
// *******************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#include "aegis/rest/rest_controller.hpp"

namespace aegis
{

namespace rest
{

AEGIS_DECL rest_controller::rest_controller(const std::string & token, asio::io_context * _io_context)
    : _token(token)
    , _io_context(_io_context)
{

}

AEGIS_DECL rest_controller::rest_controller(const std::string & token, const std::string & prefix, asio::io_context * _io_context)
    : _token(token)
    , _prefix(prefix)
    , _io_context(_io_context)
{

}

AEGIS_DECL rest_controller::rest_controller(const std::string & token, const std::string & prefix, const std::string & host, asio::io_context * _io_context)
    : _token(token)
    , _prefix(prefix)
    , _host(host)
    , _io_context(_io_context)
{

}

AEGIS_DECL rest_reply rest_controller::execute(rest::request_params && params)
{
    if (_host.empty() && params.host.empty())
        throw aegis::exception("REST host not set");

    websocketpp::http::parser::response hresponse;

    int32_t limit = 0;
    int32_t remaining = 0;
    int64_t reset = 0;
    int32_t retry = 0;
    std::chrono::system_clock::time_point http_date;
    bool global = false;

    auto start_time = std::chrono::steady_clock::now();
 
    try
    {
        asio::ip::basic_resolver<asio::ip::tcp>::results_type r;

        const std::string & tar_host = params.host.empty() ? _host : params.host;

        //TODO: make cache expire?
        auto it = _resolver_cache.find(tar_host);
        if (it == _resolver_cache.end())
        {
            asio::ip::tcp::resolver resolver(*_io_context);
            r = resolver.resolve(tar_host, params.port);
            _resolver_cache.emplace(tar_host, r);
        }
        else
            r = it->second;

        asio::ssl::context ctx(asio::ssl::context::tlsv12);

        ctx.set_options(
            asio::ssl::context::default_workarounds
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::no_sslv3);

        asio::ssl::stream<asio::ip::tcp::socket> socket(*_io_context, ctx);
        SSL_set_tlsext_host_name(socket.native_handle(), tar_host.data());

        asio::connect(socket.lowest_layer(), r);

        asio::error_code handshake_ec;
        socket.handshake(asio::ssl::stream_base::client, handshake_ec);

        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << get_method(params.method) << " " << _prefix << params.path << params._path_ex << " HTTP/1.0\r\n";
        request_stream << "Host: " << tar_host << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Authorization: Bot " << _token << "\r\n";
        request_stream << "User-Agent: DiscordBot (https://github.com/zeroxs/aegis.cpp, " << AEGIS_VERSION_LONG << ")\r\n";
        request_stream << "Connection: close\r\n";

        if (params.file.has_value())
        {
            auto & file = params.file.value();
            std::string boundary{ utility::random_string(20) };
            std::stringstream ss;

            request_stream << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";

            ss << "--" << boundary << "\r\n";
            ss << R"(Content-Disposition: form-data; name="file"; filename=")" << utility::escape_quotes(file.name) << "\"\r\n";
            ss << "Content-Type: text/plain\r\n\r\n";
            ss.write(file.data.data(), file.data.size());
            ss << "\r\n";

            ss << "--" << boundary << "--";

            request_stream << "Content-Length: " << ss.str().length() << "\r\n\r\n";
            request_stream << ss.str();
        }
        else if (!params.body.empty())
        {
            request_stream << "Content-Length: " << params.body.size() << "\r\n";
            request_stream << "Content-Type: application/json\r\n\r\n";
            request_stream << params.body;
        }

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

        http_date = utility::from_http_date(hresponse.get_header("Date")) - _tz_bias;

        global = !(hresponse.get_header("X-RateLimit-Global").empty());

#if defined(AEGIS_PROFILING)
        if (rest_end)
            rest_end(start_time, static_cast<uint16_t>(hresponse.get_status_code()));
#endif

        if (error != asio::error::eof && error != asio::ssl::error::stream_truncated)
            throw asio::system_error(error);
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return { static_cast<http_code>(hresponse.get_status_code()),
        global, limit, remaining, reset, retry, hresponse.get_body(), http_date,
        std::chrono::steady_clock::now() - start_time };
}

AEGIS_DECL rest_reply rest_controller::execute2(rest::request_params && params)
{
    if (_host.empty() && params.host.empty())
        throw aegis::exception("REST host not set");

    websocketpp::http::parser::response hresponse;

    int32_t limit = 0;
    int32_t remaining = 0;
    int64_t reset = 0;
    int32_t retry = 0;
    std::chrono::system_clock::time_point http_date;
    bool global = false;

    auto start_time = std::chrono::steady_clock::now();
    
    try
    {
        asio::ip::basic_resolver<asio::ip::tcp>::results_type r;

        const std::string & tar_host = params.host.empty() ? _host : params.host;

        //TODO: make cache expire?
        auto it = _resolver_cache.find(tar_host);
        if (it == _resolver_cache.end())
        {
            asio::ip::tcp::resolver resolver(*_io_context);
            r = resolver.resolve(tar_host, params.port);
            _resolver_cache.emplace(tar_host, r);
        }
        else
            r = it->second;


        if (params.port == "443")
        {
            asio::ssl::context ctx(asio::ssl::context::tlsv12);

            ctx.set_options(
                asio::ssl::context::default_workarounds
                | asio::ssl::context::no_sslv2
                | asio::ssl::context::no_sslv3);

            asio::ssl::stream<asio::ip::tcp::socket> socket(*_io_context, ctx);
            SSL_set_tlsext_host_name(socket.native_handle(), tar_host.data());

            asio::connect(socket.lowest_layer(), r);

            asio::error_code handshake_ec;
            socket.handshake(asio::ssl::stream_base::client, handshake_ec);

            asio::streambuf request;
            std::ostream request_stream(&request);
            request_stream << get_method(params.method) << " " << (!params.path.empty() ? params.path : "/") << " HTTP/1.0\r\n";
            request_stream << "Host: " << tar_host << "\r\n";
            request_stream << "Accept: */*\r\n";
            for (auto & h : params.headers)
                request_stream << h << "\r\n";
            request_stream << "Connection: close\r\n";

            if (!params.body.empty())
            {
                request_stream << "Content-Length: " << params.body.size() << "\r\n";
                request_stream << "Content-Type: application/json\r\n\r\n";
                request_stream << params.body;
            }

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

            http_date = utility::from_http_date(hresponse.get_header("Date"));

            if (error != asio::error::eof && error != asio::ssl::error::stream_truncated)
                throw asio::system_error(error);

            //TODO: return reply headers
        }
        else
        {
            asio::ip::tcp::socket socket(*_io_context);
            asio::connect(socket, r);

            asio::streambuf request;
            std::ostream request_stream(&request);
            request_stream << get_method(params.method) << " " << (!params.path.empty() ? params.path : "/") << " HTTP/1.0\r\n";
            request_stream << "Host: " << tar_host << "\r\n";
            request_stream << "Accept: */*\r\n";
            for (auto & h : params.headers)
                request_stream << h << "\r\n";

            request_stream << "Connection: close\r\n";
          
            if (!params.body.empty())
            {
                request_stream << "Content-Length: " << params.body.size() << "\r\n";
                request_stream << "Content-Type: application/json\r\n\r\n";
                request_stream << params.body;
            }

            asio::write(socket, request);
            asio::streambuf response;
            asio::read_until(socket, response, "\r\n");
            std::stringstream response_content;
            response_content << &response;

            std::istringstream istrm(response_content.str());
            hresponse.consume(istrm);

            http_date = utility::from_http_date(hresponse.get_header("Date"));

            //TODO: return reply headers
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return { static_cast<http_code>(hresponse.get_status_code()),
        global, limit, remaining, reset, retry, hresponse.get_body(), http_date,
        std::chrono::steady_clock::now() - start_time };
}

}

}
