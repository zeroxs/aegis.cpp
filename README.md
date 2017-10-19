[![Build Status](https://travis-ci.org/zeroxs/aegis.cpp.svg?branch=master)](https://travis-ci.org/zeroxs/aegis.cpp) [![Discord](https://discordapp.com/api/guilds/287048029524066334/widget.png)](https://discord.gg/w7Y3Bb8) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)


Aegis Library
=======

C++17 library for interfacing with the [Discord API](https://discordapp.com/developers/docs/intro)

# License #

This project is licensed under the MIT license. See [LICENSE](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)

Libraries used:
- [Asio](https://github.com/chriskohlhoff/asio)
- [Websocketpp](https://github.com/zaphoyd/websocketpp)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog)
- [OpenSSL](https://www.openssl.org)
- [zlib](https://zlib.net)
- [zstr](https://github.com/mateidavid/zstr)



# TODO #
- Voice data send/recv


# Using this library #
`#include "aegis/aegis.hpp"`

Quick and dirty g++-7 command
`g++-7 -std=c++17 -Iinclude -Ilib/spdlog/include -Ilib/websocketpp -Ilib/asio/asio/include -Ilib/json/src -Ilib/zstr/src src/main.cpp -lssl -lpthread -lcrypto -lz -o aegis`

Visual Studio solution inside `project/`
