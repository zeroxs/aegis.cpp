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

#include <aegis/aegis.hpp>

using aegis::Aegis;

int main(int argc, char * argv[])
{
    asio::io_service io_service;

    if (argc <= 5)
        return -1;

    const int TRAVIS_BUILD_NUMBER = std::stoi(argv[2]);
    const std::string TRAVIS_PULL_REQUEST_SHA(argv[3]);
    const bool TRAVIS_SECURE_ENV_VARS = (!memcmp(argv[4], "true", 4));
    const uint64_t channel_id = std::stoull(argv[5]);

    try
    {
        Aegis bot(argv[1]);

        bot.initialize(&io_service);

        if (!TRAVIS_SECURE_ENV_VARS)
        {
            std::cout << "PULL REQUEST\n";
            return 0;
        }

        json obj =
        {
            { "content", fmt::format("Aegis library build {}.{}.{}\n\nBuild number: {}\nBuild hash: {}", AEGIS_VERSION_MAJOR, AEGIS_VERSION_MINOR, AEGIS_VERSION_REVISION, TRAVIS_BUILD_NUMBER, TRAVIS_PULL_REQUEST_SHA) }
        };

        std::thread thd([&] { bot.run(); });

        std::optional<aegis::rest_reply> reply = bot.post(fmt::format("/channels/{}/messages", channel_id), obj.dump());
        if (reply.has_value() && reply->reply_code == 200)
            return 0;

        return 1;
    }
    catch (std::exception & e)
    {
        std::cout << fmt::format("Uncaught error: {0}", e.what());
        return 1;
    }
    return 0;
}


