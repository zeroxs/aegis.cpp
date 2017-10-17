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


#include <stdint.h>
#include <aegis/aegis.hpp>
#include <aegis/client.hpp>
#include <aegis/snowflake.hpp>
using namespace aegis;
using json = nlohmann::json;
int main(int argc, char * argv[])
{
    Aegis bot("TOKEN");
    bot.i_message_create = [] (json & msg, client & shard, Aegis & bot) -> bool
    {
        snowflake channel_id = std::stoll(msg["d"]["channel_id"].get<std::string>());
        std::string author = msg["d"]["author"];
        if (msg["d"]["content"] == "Hi")
            bot.get_channel(channel_id).create_message("Hello back");
        return true;
    };
    bot.easy_start();
    return 0;
}


