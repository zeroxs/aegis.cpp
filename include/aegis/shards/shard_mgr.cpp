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
    set_state(Ready);
    ws_open_strand = std::make_unique<asio::io_context::strand>(_io_context);
}

AEGIS_DECL shard_mgr::~shard_mgr()
{
    for (auto & _shard : _shards)
    {
        _shard->cleanup();
    }
}

AEGIS_DECL void shard_mgr::start(std::size_t count)
{
    if (count == 0)
        count = std::thread::hardware_concurrency();

    // ensure any sort of single blocking call in message processing usercode doesn't block everything
    // this will not protect against faulty usercode entirely, but will at least provide some leeway
    // to allow a single blocking message to not halt operations
    if (count == 1)
        count = 2;

    set_state(Ready);

    if (gateway_url.empty())
        throw exception(error::get_gateway);

    wrk = std::make_shared<asio_exec>(asio::make_work_guard(_io_context));
    for (std::size_t i = 0; i < count; ++i)
        threads.emplace_back(std::bind(static_cast<asio::io_context::count_type(asio::io_context::*)()>(&asio::io_context::run), &_io_context));

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

            log->trace("Shard#{}: added to connect list", _shard->get_id());
            _shards_to_connect.push_back(_shard.get());
            _shards.push_back(std::move(_shard));
        }

        ws_timer = websocket_o.set_timer(100, std::bind(&shard_mgr::ws_status, this, std::placeholders::_1));

        std::unique_lock<std::mutex> l(m);
        cv.wait(l);

        log->info("Closing bot");
    }
}

AEGIS_DECL void shard_mgr::stop()
{
    wrk.reset();
    _io_context.stop();
    for (auto & t : threads)
        t.join();
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
    set_state(Shutdown);
    websocket_o.stop();
    cv.notify_all();
    for (auto & _shard : _shards)
    {
        _shard->cleanup();
        _shard->connection_state = Shutdown;
        if (_shard->_connection && _shard->_connection->get_socket().lowest_layer().is_open())
            _shard->_connection->close(1001, "");
        _shard->do_reset();
    }
}

AEGIS_DECL std::string shard_mgr::uptime() const AEGIS_NOEXCEPT
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
            log->trace("Shard#{}: zlib-stream incomplete", _shard->shardid);
            return;
        }

        try
        {
            std::stringstream ss;
            std::string s;
            //DEBUG
            if (_shard->zlib_ctx == nullptr)
            {
                log->error("Shard#{}: zlib failure. Context null.", _shard->shardid);
                _shard->close();
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
            log->error("Shard#{}: zlib failure. Context invalid. {}", _shard->shardid, e.what());
            _shard->close();
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

    if (i_on_message)
        i_on_message(hdl, payload, _shard);
}

AEGIS_DECL void shard_mgr::_on_connect(websocketpp::connection_hdl hdl, shard * _shard)
{
    log->info("Shard#{}: connection established", _shard->shardid);
    _shard->set_connected();
    if (_shards_to_connect.empty())
    {
        log->error("Shard#{}: _shards_to_connect empty", _shard->shardid);
    }
    else
    {
        auto * _s = _shards_to_connect.front();
        assert(_s != nullptr);
        if (_s != _shard)
            log->error("Shard#{}: _shards_to_connect.front wrong shard id:{} status:{}",
                       _shard->shardid,
                       _s->shardid,
                       _s->is_connected()?"connected":"not connected");
        if (_s != _connecting_shard)
            log->error("Shard#{}: _connecting_shard wrong shard id:{} status:{}",
                       _shard->shardid,
                       _s->shardid,
                       _s->is_connected() ? "connected" : "not connected");
        _shards_to_connect.pop_front();
    }

    if (i_on_connect)
        i_on_connect(hdl, _shard);
}

AEGIS_DECL void shard_mgr::_on_close(websocketpp::connection_hdl hdl, shard * _shard)
{
    if (_status == Shutdown)
    {
        reset_shard(_shard);
        return;
    }
    auto now = std::chrono::steady_clock::now();
    log->error("Shard#{}: disconnected. lastheartbeat({}) lastwsevent({}) lastheartbeatack({}) ms ago",
               _shard->get_id(),
               std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->lastheartbeat).count(),
               std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->lastwsevent).count(),
               std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->heartbeat_ack).count());
    reset_shard(_shard);

    //TODO debug only auto reconnect 50 times per session
    if (_shard->counters.reconnects < 50)
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

AEGIS_DECL void shard_mgr::reset_shard(shard * _shard)
{
    _shard->do_reset();
    _shard->connection_state = bot_status::Reconnecting;
}

AEGIS_DECL void shard_mgr::ws_status(const asio::error_code & ec)
{
    if ((ec == asio::error::operation_aborted) || (_status == Shutdown))
        return;

    using namespace std::chrono_literals;

    try
    {
        auto now = std::chrono::steady_clock::now();

        // do basic shard timer actions such as timing out potential ghost sockets
        for (auto & _shard : _shards)
        {
            if (_shard == nullptr)
                continue;

            if (_shard->is_connected())
            {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(_shard->lastwsevent.time_since_epoch()).count() > 0)
                {
                    // heartbeat system should typically pick up any dead sockets. this is sort of redundant at the moment
                    if (_shard->lastwsevent < (now - 90s))
                    {
                        log->error("Shard#{}: Websocket had no events in last 90s - reconnecting", _shard->shardid);
                        debug_trace(_shard.get());
                        if (_shard->_connection->get_state() < websocketpp::session::state::closing)
                        {
                            websocket_o.close(_shard->_connection, 1001, "");
                            _shard->connection_state = bot_status::Reconnecting;
                        }
                        else
                            reset_shard(_shard.get());
                    }
                }
            }
            _shard->last_status_time = now;
        }

        // check if not all shards connected
        if (!_shards_to_connect.empty())
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - _last_ready) >= 6s)
            {
                if (_connect_time != std::chrono::steady_clock::time_point())
                {
                    if (std::chrono::duration_cast<std::chrono::seconds>(now - _connect_time) >= 10s)
                    {
                        log->error("Shard#{}: timeout while connecting (10s)", _connecting_shard->get_id());
                        _shards_to_connect.pop_front();
                        reset_shard(_connecting_shard);
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
                        {
                            throw ec;
                        }

                        _shard->connect();
                        setup_callbacks(_shard);
                        _shard->connection_state = bot_status::Connecting;
                    }
                    else
                    {
                        log->debug("Shard#{}: already connected {} {} {} {}",
                                    _shard->get_id(),
                                    _shard->_connection->get_state(),
                                    _shard->connection_state,
                                    std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->lastwsevent).count(),
                                    std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->last_status_time).count());
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
    ws_timer = websocket_o.set_timer(1000, std::bind(&shard_mgr::ws_status, this, std::placeholders::_1));
}

AEGIS_DECL void shard_mgr::queue_reconnect(shard * _shard)
{
    auto it = std::find(_shards_to_connect.cbegin(), _shards_to_connect.cend(), _shard);
    if (it != _shards_to_connect.cend())
    {
        log->error("Shard#{}: shard to be connected already on connect list", _shard->shardid);
        return;
    }
    log->trace("Shard#{}: added to connect list", _shard->get_id());
    _shards_to_connect.push_back(_shard);
}

AEGIS_DECL void shard_mgr::debug_trace(shard * _shard, bool extended)
{
    fmt::MemoryWriter w;

    w << "~~ERROR~~ extended(" << extended << ")"
        << "\n==========<Start Error Trace>==========\n"
        << "Shard: " << _shard->shardid << '\n'
        << "Seq: " << _shard->sequence << '\n';
    int i = 0;

    for (auto & msg : _shard->debug_messages)
        w << std::get<0>(msg) << " - " << std::get<1>(msg) << '\n';

    /// in most cases the entire shard list shouldn't be dumped
    if (extended)
    for (auto & c : _shards)
    {
        if (c == nullptr)
            w << fmt::format("Shard#{} INVALID OBJECT\n",
                             _shard->shardid);

        else
        {
            if (c->_connection == nullptr)
                w << fmt::format("Shard#{} not connected\n",
                                 c->shardid);
            else
                w << fmt::format("Shard#{} connected Seq:{} LastWS:{}ms ago\n",
                                 c->shardid, c->sequence, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - c->lastwsevent).count());
        }
    }
    w << "==========<End Error Trace>==========";
    log->error(w.str());
}

AEGIS_DECL shard & shard_mgr::get_shard(uint16_t shard_id)
{
    if (shard_id >= _shards.size())
        throw std::out_of_range("shard_mgr::get_shard out of range error");
    return *_shards[shard_id];
}

}

}
