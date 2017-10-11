[![Build Status](https://travis-ci.org/zeroxs/aegis.cpp.svg?branch=master)](https://travis-ci.org/zeroxs/aegis.cpp) [![Discord](https://discordapp.com/api/guilds/287048029524066334/widget.png)](https://discord.gg/w7Y3Bb8) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)


Aegis Library
=======

C++17 implementation for the [Discord API](https://discordapp.com/developers/docs/intro)

# License #

This project is licensed under the MIT license. See [LICENSE](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)

Libraries used:
- [Websocketpp](https://github.com/zaphoyd/websocketpp)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog)
- [OpenSSL](https://www.openssl.org)
- [zlib](https://zlib.net)



# TODO #
- Finish guild/channel/member/command implementations
- Voice data send/recv


# Using this library #
`#include "aegis/aegis.hpp"`

Quick and dirty g++-7 command
`g++-7 -std=c++17 -Iinclude -Ilib/websocketpp/ -Ilib/asio/include -Ilib/json/src src/main.cpp -lssl -lpthread -lcrypto`

Visual Studio solution inside `project/`
