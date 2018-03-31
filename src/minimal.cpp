//
// minimal.cpp
// aegis.cpp
//
// Copyright (c) 2017 Sara W (sara at xandium dot net)
//
// This file is part of aegis.cpp .
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <aegis.hpp>
using namespace aegiscpp;
using json = nlohmann::json;
int main(int argc, char * argv[])
{
    aegis bot("TOKEN");
    callbacks cbs;
    std::mutex m_ping_test;
    std::condition_variable cv_ping_test;
    int64_t ws_checktime = 0;
    cbs.i_message_create = [&](message_create obj)
    {
        const auto &[channel_id, guild_id, message_id, member_id] = obj.msg.get_related_ids();
       
        //ping test
        static snowflake ping_check(0);
        static int64_t checktime(0);

        if (obj.get_member().get_id() == obj.bot->self()->get_id())
        {
            if (obj.msg.nonce == checktime)
            {
                {
                    std::lock_guard<std::mutex> lk(m_ping_test);
                    ws_checktime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - checktime;
                }
                cv_ping_test.notify_all();
                return;
            }
        }


        if (obj.msg.is_bot() || !obj.has_member() || !obj.has_channel())
            return;

        auto & _member = obj.get_member();
        auto & _channel = obj.get_channel();
        auto & _guild = _channel.get_guild();

        auto & username = _member.name;

        std::string content{ obj.msg.get_content() };

        // Simple Hi response
        if (content == "Hi")
        {
            _channel.create_message("Hello back");
        }
        // Complex ping reply. Edits bot's message to display REST response time
        // Then when the Websocket receives the message, edits it again to show the time
        // the message took to come over the websocket
        else if (content == "ping")
        {
            std::unique_lock<std::mutex> lk(m_ping_test);
            checktime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            auto apireply = _channel.create_message("Pong").get();
            if (apireply.reply_code == 200)
            {
                bot.log->info(apireply.content);
                message msg = json::parse(apireply.content);
                msg.init(obj._shard);
                std::string to_edit = fmt::format("Ping reply: REST [{}ms]", (duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - checktime));
                msg.edit(to_edit);
                if (cv_ping_test.wait_for(lk, 20s) == std::cv_status::no_timeout)
                {
                    msg.edit(fmt::format("{} WS [{}ms]", to_edit, ws_checktime));
                    return;
                }
                else
                {
                    msg.edit(fmt::format("{} WS [timeout20s]", to_edit));
                    return;
                }
            }
        }

        return;
    };
    bot._callbacks = cbs;
    bot.easy_start();
    return 0;
}


