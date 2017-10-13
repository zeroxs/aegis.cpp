//
// main.cpp
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


#include "example.hpp"
#include <aegis/aegis.hpp>


using aegis::Aegis;
using example_bot::example;


int main(int argc, char * argv[])
{
    auto err = spd::stdout_color_mt("err");
    spd::set_pattern("%Y-%m-%d %H:%M:%S.%e %l [th#%t] : %v");

    
    try
    {
        // Create bot with token
        Aegis<aegis::basebot> bot("TOKEN");
        
        bot.self_presence = { { "game",{ { "name", "@Aegis help" },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } };
        
        example<aegis::basebot> commands;

        commands.inject(bot);



        // Configure everything and run bot
        bot.easy_start();
    }
    catch (std::exception & e)
    {
        err->error(fmt::format("Uncaught error: {0}", e.what()));
    }
    return 0;
}


