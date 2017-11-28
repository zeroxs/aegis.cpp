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
    cbs.i_message_create = [&](message_create obj) -> bool
    {
        std::string username;
        auto _member = obj._member;
        if (_member != nullptr)
            username = obj._member->name;

        auto _channel = obj._channel;
        auto & _guild = _channel->get_guild();

        snowflake channel_id = _channel->channel_id;
        snowflake message_id = obj.msg.message_id;
        std::string content = obj.msg.content;

        if (content == "Hi")
            bot.get_channel(channel_id)->create_message("Hello back");
        return true;
    };
    bot._callbacks = cbs;
    bot.easy_start();
    return 0;
}


