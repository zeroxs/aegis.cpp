//
// shard_mgr.cpp
// *************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#include "aegis/shards/shard_mgr.hpp"
#include <string>
#include <asio/streambuf.hpp>
#include <asio/connect.hpp>

#include <nlohmann/json.hpp>

namespace aegis
{

namespace shards
{

AEGIS_DECL shard_mgr::shard_mgr(std::string token, asio::io_context & _io, std::shared_ptr<spdlog::logger> log)
    : _io_context(_io)
    , force_shard_count(0)
    , shard_max_count(0)
    , log(log)
    , _connecting_shard(nullptr)
    , _token(token)
{
    websocket_o.init_asio(&_io_context);

    websocket_o.clear_access_channels(websocketpp::log::alevel::all);
    websocket_o.clear_error_channels(websocketpp::log::alevel::all);

    websocket_o.set_tls_init_handler([](websocketpp::connection_hdl)
    {
        return websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
    });
    starttime = std::chrono::steady_clock::now();
    set_state(bot_status::Running);
    //ws_open_strand = std::make_unique<asio::io_context::strand>(_io_context);
}

AEGIS_DECL shard_mgr::~shard_mgr()
{

}

AEGIS_DECL void shard_mgr::start()
{
    set_state(bot_status::Running);

    if (gateway_url.empty())
        throw exception(error::get_gateway);

    starttime = std::chrono::steady_clock::now();
    
    log->info("Starting bot with {} shards", shard_max_count);
    {
        log->info("Websocket[s] connecting");
        for (uint32_t k = 0; k < shard_max_count; ++k)
        {
            std::error_code ec;
            auto _shard = std::make_unique<aegis::shards::shard>(_io_context, websocket_o, k);

            if (ec)
            {
                log->critical("Websocket connection failed: {}", ec.message());
                return;
            }

            AEGIS_TRACE(log, "Shard#{}: added to connect list", _shard->get_id());
            _shards_to_connect.push_back(_shard.get());
            _shards.push_back(std::move(_shard));
        }

        ws_timer = websocket_o.set_timer(100, std::bind(&shard_mgr::ws_status, this, std::placeholders::_1));
    }
}

AEGIS_DECL void shard_mgr::setup_callbacks(shard * _shard)
{
    _shard->_connection->set_message_handler(
        std::bind(&shard_mgr::_on_message, this, std::placeholders::_1, std::placeholders::_2, _shard));
    _shard->_connection->set_open_handler(
        std::bind(&shard_mgr::_on_connect, this, std::placeholders::_1, _shard));
    _shard->_connection->set_close_handler(
        std::bind(&shard_mgr::_on_close, this, std::placeholders::_1, _shard));
}

AEGIS_DECL void shard_mgr::shutdown()
{
    set_state(bot_status::Shutdown);
    websocket_o.stop();
    for (auto & _shard : _shards)
        _shard->do_reset(shard_status::Shutdown);
}

AEGIS_DECL std::string shard_mgr::uptime() const noexcept
{
    using seconds = std::chrono::duration<int, std::ratio<1, 1>>;
    using minutes = std::chrono::duration<int, std::ratio<60, 1>>;
    using hours = std::chrono::duration<int, std::ratio<3600, 1>>;
    using days = std::chrono::duration<int, std::ratio<24 * 3600, 1>>;

    std::stringstream ss;
    std::chrono::time_point<std::chrono::steady_clock> now_t = std::chrono::steady_clock::now();

    auto time_is = now_t - starttime;
    auto d = std::chrono::duration_cast<days>(time_is).count();
    time_is -= days(d);
    auto h = std::chrono::duration_cast<hours>(time_is).count();
    time_is -= hours(h);
    auto m = std::chrono::duration_cast<minutes>(time_is).count();
    time_is -= minutes(m);
    auto s = std::chrono::duration_cast<seconds>(time_is).count();

    if (d)
        ss << d << "d ";
    if (h)
        ss << h << "h ";
    if (m)
        ss << m << "m ";
    ss << s << "s ";
    return ss.str();
}

AEGIS_DECL void shard_mgr::_on_message(websocketpp::connection_hdl hdl, message_ptr msg, shard * _shard)
{
    _shard->transfer_bytes += msg->get_header().size() + msg->get_payload().size();
    _shard->transfer_bytes_u += msg->get_header().size();

    _shard->lastwsevent = std::chrono::steady_clock::now();

    std::string payload;

    try
    {
        //zlib detection and decoding
        _shard->ws_buffer.clear();
        _shard->ws_buffer.str("");
        _shard->ws_buffer << msg->get_payload();

        const std::string & pld = msg->get_payload();
        if (std::strcmp((pld.data() + pld.size() - 4), "\x00\x00\xff\xff"))
        {
            log->error("Shard#{}: zlib-stream incomplete", _shard->get_id());
            return;
        }

        try
        {
            std::stringstream ss;
            std::string s;
            //DEBUG
            if (_shard->zlib_ctx == nullptr)
            {
                log->error("Shard#{}: zlib failure. Context null.", _shard->get_id());
                close(*_shard, 1001, "", aegis::shard_status::Reconnecting);
                return;
            }
            _shard->zlib_ctx->clear();
            while (getline(*_shard->zlib_ctx, s))
                ss << s;
            ss << '\0';
            payload = ss.str();
            _shard->transfer_bytes_u += payload.size();
        }
        catch (std::exception & e)
        {
            log->error("Shard#{}: zlib failure. Context invalid. {}", _shard->get_id(), e.what());
            close(*_shard, 1001, "", aegis::shard_status::Reconnecting);
            return;
        }
    }
    catch (std::exception& e)
    {
        log->error("Failed to process object: {0}", e.what());
        log->error(payload);

        debug_trace(_shard);
    }
    catch (...)
    {
        log->error("Failed to process object: Unknown error");
        debug_trace(_shard);
    }

#if defined(AEGIS_DEBUG_HISTORY)
    _shard->debug_messages.emplace_back(std::tuple<std::chrono::steady_clock::time_point, std::string>{ std::chrono::steady_clock::now(), payload });
#endif

    if (i_on_message)
        i_on_message(hdl, std::move(payload), _shard);
}

AEGIS_DECL void shard_mgr::_on_connect(websocketpp::connection_hdl hdl, shard * _shard)
{
    log->info("Shard#{}: connection established", _shard->get_id());
    _shard->set_connected();
    if (_shards_to_connect.empty())
    {
        log->error("Shard#{}: _shards_to_connect empty", _shard->get_id());
    }
    else
    {
        if (_connecting_shard == nullptr)
            return;
        auto * _s = _shards_to_connect.front();
        assert(_s != nullptr);
        if (_s != _shard)
            log->error("Shard#{}: _shards_to_connect.front wrong shard id:{} status:{}",
                       _shard->get_id(),
                       _s->get_id(),
                       _s->is_connected()?"connected":"not connected");
        if (_s != _connecting_shard)
            log->error("Shard#{}: _connecting_shard wrong shard id:{} status:{}",
                       _shard->get_id(),
                       _s->get_id(),
                       _s->is_connected() ? "connected" : "not connected");
        _shards_to_connect.pop_front();
    }

    if (i_on_connect)
        i_on_connect(hdl, _shard);
}

AEGIS_DECL void shard_mgr::_on_close(websocketpp::connection_hdl hdl, shard * _shard)
{
    if (_connecting_shard == _shard)
        _connect_time = std::chrono::steady_clock::time_point();
    _shard->connect_time = std::chrono::steady_clock::time_point();
    if (_status == bot_status::Shutdown || _shard->connection_state == shard_status::Shutdown)
    {
        if (i_on_close)
            i_on_close(hdl, _shard);
        reset_shard(_shard);
        return;
    }

    auto now = std::chrono::steady_clock::now();
    log->error("Shard#{}: disconnected. lastheartbeat({}) lastwsevent({}) lastheartbeatack({}) ms ago",
               _shard->get_id(),
               utility::to_ms(now - _shard->lastheartbeat),
               utility::to_ms(now - _shard->lastwsevent),
               utility::to_ms(now - _shard->heartbeat_ack));
    reset_shard(_shard, shard_status::Queued);

    //TODO debug only auto reconnect 50 times per session
    //instead, check how quickly reconnects are happening to identify a connect loop
    //if (_shard->counters.reconnects < 50)
    queue_reconnect(_shard);

    if (i_on_close)
        i_on_close(hdl, _shard);
}

AEGIS_DECL void shard_mgr::send_all_shards(const std::string & msg)
{
    for (auto & s : _shards)
        s->send(msg);
}

AEGIS_DECL void shard_mgr::send_all_shards(const json & msg)
{
    for (auto & s : _shards)
        s->send(msg.dump());
}

AEGIS_DECL void shard_mgr::reset_shard(shard * _shard, shard_status _status)
{
    _shard->do_reset(_status);
}

AEGIS_DECL void shard_mgr::ws_status(const asio::error_code & ec)
{
    if ((ec == asio::error::operation_aborted) || (_status == bot_status::Shutdown))
        return;

    ws_timer = websocket_o.set_timer(100, std::bind(&shard_mgr::ws_status, this, std::placeholders::_1));

    using namespace std::chrono_literals;

    try
    {
        auto now = std::chrono::steady_clock::now();

        // do basic shard timer actions such as timing out potential ghost sockets
        for (auto & _shard : _shards)
        {
            if (_shard == nullptr || _shard->connection_state == shard_status::Uninitialized)
                continue;

            if (_shard->is_connected())
            {
                // a try should the 'worst' happen
                try
                {
                    if (utility::to_ms(_shard->lastwsevent) > 0)
                    {
                        // heartbeat system should typically pick up any dead sockets. this is sort of redundant at the moment
                        if (_shard->lastwsevent < (now - 90s))
                        {
                            _shard->lastwsevent = now;
                            log->error("Shard#{}: Websocket had no events in last 90s - reconnecting", _shard->get_id());
                            debug_trace(_shard.get());
                            reset_shard(_shard.get());
                        }
                    }
                }
                catch (std::exception & e)
                {
                    log->error("Shard#{}: worst happened std::exception {}", _shard->get_id(), e.what());
                    reset_shard(_shard.get());
                }
                catch (asio::error_code & e)
                {
                    log->error("Shard#{}: worst happened asio::error_code {}:{}", _shard->get_id(), e.value(), e.message());
                    reset_shard(_shard.get());
                }
            }
            else
            {
                if (_shard->connection_state == shard_status::Closed)
                {
                    queue_reconnect(*_shard);
                }
                // shard in process of closing. check for close timeout then force close
                //TODO: test this?
                else if (_shard->connection_state == shard_status::Closing
                         && utility::to_ms(now - _shard->closing_time) > 5000)
                {
                    std::error_code ec;
                    _shard->_connection->terminate(ec);
                    _shard->_connection.reset();
                    _shard->do_reset();
                }
                // shard is not connected. do a check if shard is in connection queue or if it needs adding to it
                else if (_shard->connection_state == shard_status::Reconnecting &&
                    _shard->connect_time != std::chrono::steady_clock::time_point() &&
                    utility::to_ms(now - _shard->connect_time) > 20000)
                {
                    close(_shard.get());
                    log->error("Shard#{}: shard timed out reconnecting (20s)", _shard->get_id());
                    reset_shard(_shard.get());
                }
//                 if (_shard->connection_state != shard_status::Shutdown
//                     && _shard->connection_state != shard_status::Connecting
//                     && _shard->connection_state != shard_status::Reconnecting)
//                 {
//                     queue_reconnect(_shard.get());
//                 }
            }
            _shard->last_status_time = now;
        }

        // check if not all shards connected
        //TODO: speed clear this list if shard is in resume state
        if (!_shards_to_connect.empty())
        {
            if (utility::to_t<std::chrono::seconds>(now - _last_identify) >= 6s)
            {
                if (_connect_time != std::chrono::steady_clock::time_point())
                {
                    if (utility::to_t<std::chrono::seconds>(now - _connect_time) >= 20s)
                    {
                        log->error("Shard#{}: timeout while connecting (20s)", _connecting_shard->get_id());
                        close(_connecting_shard);
                        _shards_to_connect.pop_front();
                        queue_reconnect(_connecting_shard);
                        _connecting_shard = nullptr;
                        _connect_time = std::chrono::steady_clock::time_point();
                    }
                }
                else
                {
                    log->info("Shards to connect : {}", _shards_to_connect.size());
                    auto * _shard = _shards_to_connect.front();

                    if (!_shard->is_connected())
                    {
                        log->info("Shard#{}: connecting", _shard->get_id());
                        _connecting_shard = _shard;
                        _connect_time = now;

                        asio::error_code ec;
                        _shard->_connection = websocket_o.get_connection(gateway_url, ec);
                        if (ec)
                            throw ec;

                        _shard->connection_state = shard_status::Reconnecting;
                        connect(_shard);
                    }
                    else
                    {
                        AEGIS_DEBUG(log, "Shard#{}: already connected {} {} {} {}",
                                   _shard->get_id(),
                                   _shard->_connection->get_state(),
                                   static_cast<int>(_shard->connection_state),
                                   utility::to_ms(now - _shard->lastwsevent),
                                   utility::to_ms(now - _shard->last_status_time));
                        _shards_to_connect.pop_front();
                    }
                }
            }
        }
    }
    catch (std::exception & e)
    {
        log->error("ws_status() error : {}", e.what());
    }
    catch (...)
    {
        log->error("ws_status() error : unknown");
    }
}

AEGIS_DECL void shard_mgr::connect(shard * _shard)
{
    asio::post(asio::bind_executor(*_shard->_connection->get_strand(), [this, _shard]()
    {
        setup_callbacks(_shard);
        _shard->connection_state = shard_status::Connecting;
        _shard->lastheartbeat =
            _shard->last_status_time =
            _shard->lastwsevent =
            std::chrono::steady_clock::now();
        _shard->connect();
    }));
}

AEGIS_DECL void shard_mgr::queue_reconnect(shard * _shard)
{
    auto it = std::find(_shards_to_connect.cbegin(), _shards_to_connect.cend(), _shard);
    if (it != _shards_to_connect.cend())
    {
        log->error("Shard#{}: shard to be connected already on connect list", _shard->get_id());
        return;
    }
    AEGIS_DEBUG(log, "Shard#{}: added to connect list", _shard->get_id());
    //_shard->connection_state = shard_status::Queued;
    _shards_to_connect.push_back(_shard);
}

AEGIS_DECL void shard_mgr::debug_trace(shard * _shard, bool extended)
{
#if defined(AEGIS_DEBUG_HISTORY)
    fmt::MemoryWriter w;

    w << "~~ERROR~~ extended(" << extended << ")"
        << "\n==========<Start Error Trace>==========\n"
        << "Shard: " << _shard->get_id() << '\n'
        << "Seq: " << _shard->get_sequence() << '\n';

    for (auto & msg : _shard->debug_messages)
        w << std::get<0>(msg) << " - " << std::get<1>(msg) << '\n';

    /// in most cases the entire shard list shouldn't be dumped
    if (extended)
    for (auto & c : _shards)
    {
        if (c == nullptr)
            w << fmt::format("Shard#{} INVALID OBJECT\n",
                             _shard->get_id());

        else
        {
            if (c->_connection == nullptr)
                w << fmt::format("Shard#{} not connected\n",
                                 c->get_id());
            else
                w << fmt::format("Shard#{} connected Seq:{} LastWS:{}ms ago\n",
                                 c->get_id(), c->get_sequence(), utility::to_ms(std::chrono::steady_clock::now() - c->lastwsevent));
        }
    }
    w << "==========<End Error Trace>==========";
    log->error(w.str());
#endif
}

AEGIS_DECL shard & shard_mgr::get_shard(uint16_t shard_id)
{
    if (shard_id >= _shards.size())
        throw std::out_of_range("shard_mgr::get_shard out of range error");
    return *_shards[shard_id];
}

AEGIS_DECL void shard_mgr::close(shard * _shard, int32_t code, const std::string & reason, shard_status connection_state)
{
    _shard->connection_state = connection_state;
    _shard->closing_time = std::chrono::steady_clock::now();
    if (_shard->_connection != nullptr && _shard->_connection->get_raw_socket().is_open())
    {
        asio::error_code ec;
        _shard->_connection->close(code, reason, ec);
    }
}

}

}
