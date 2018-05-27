[![Build Status](https://travis-ci.org/zeroxs/aegis.cpp.svg?branch=master)](https://travis-ci.org/zeroxs/aegis.cpp) [![Discord](https://discordapp.com/api/guilds/287048029524066334/widget.png)](https://discord.gg/w7Y3Bb8) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)


Aegis Library
=======

C++14/17 library for interfacing with the [Discord API](https://discordapp.com/developers/docs/intro)

# License #

This project is licensed under the MIT license. See [LICENSE](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)

Libraries used (all are header-only with the exception of zlib):
- [Asio](https://github.com/chriskohlhoff/asio)
- [Websocketpp](https://github.com/zaphoyd/websocketpp)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog) (by extension, [fmtlib](https://github.com/fmtlib/fmt))
- [OpenSSL 1.0.2](https://www.openssl.org)
- [zlib](https://zlib.net)
- [zstr](https://github.com/mateidavid/zstr)



# TODO #
- Voice data send/recv
- Finish documentation
- Finish live example of library in use
- Finish remaining API endpoints

# Documentation #
You can access the [documentation here](https://docs.aegisbot.io/). It is a work in progress itself and has some missing parts, but most of the library is now documented.

# Using this library #
There are multiple ways to make use of this library.

#### Header only ####
Including the helper header will automatically include all other files.
```cpp
#include <aegis.hpp>

int main()
{
    aegis::core bot;
    bot.i_message_create = [](auto obj)
    {
        if (obj.msg.get_content() == "Hi")
            obj.msg.get_channel().create_message(fmt::format("Hello {}", obj.msg.author.username));
    };
    bot.run();
}
```

#### Separate compilation ####
You can include `#include <aegis/src.hpp>` within a single cpp file while defining `-DAEGIS_SEPARATE_COMPILATION`

#### Shared/Static library ####
You can build this library with CMake.
```
$ git clone --recursive https://github.com/zeroxs/aegis.cpp.git
$ cd aegis.cpp
$ mkdir build
$ cd build
$ cmake ..
// or to use C++17
$ cmake -DCMAKE_CXX_COMPILER=g++-7 -DCXX_STANDARD=17 ..
```
You can also add `-DBUILD_EXAMPLES=1` and it will build 3 examples within the ./src directory.</br>
`example_main.cpp;example.cpp` will build a bot that runs out of its own class</br>
`minimal.cpp` will build two versions, one (aegis_minimal) will be with the shared/static library. The other (aegis_headeronly_no_cache) will be header-only but the lib will store no internal cache.


## Compiler Options ##
You can pass these flags to CMake to change what it builds</br>
`-DBUILD_EXAMPLES=1` will build the examples</br>
`-DCMAKE_CXX_COMPILER=g++-7` will let you select the compiler used</br>
`-DCXX_STANDARD=17` will let you select C++14 (default) or C++17

##### Library #####
You can pass these flags to your compiler to alter how the library is built</br>
`-DAEGIS_DISABLE_ALL_CACHE` will disable the internal caching of most objects such as member data reducing memory usage by a significant amount</br>
`-DAEGIS_DEBUG_HISTORY` enables the saving of the last 5 messages sent on the shard's websocket. In the event of an uncaught exception, they are dumped to console.</br>
`-DAEGIS_PROFILING` by setting up to 3 specific functions within the main class:
```cpp
bot.message_end = std::bind(&AegisBot::message_end, this, std::placeholders::_1, std::placeholders::_2);
bot.js_end = std::bind(&AegisBot::js_end, this, std::placeholders::_1, std::placeholders::_2);
bot.call_end = std::bind(&AegisBot::call_end, this, std::placeholders::_1);
```
your callbacks will be executed:
- `message_end`: when a whole message is done being processed including your own callback time but not including js decode time
- `js_end`: when a js decoding is completed
- `call_end`: when a rest call function is completed

</br></br>
`message_end` and `js_end` both are passed 2 parameters ``(std::chrono::steady_clock::time_point, const std::string &)`` while `call_end` is only passed `(std::chrono::steady_clock::time_point)`. The time_point being passed is the time the action started. The string being the websocket event name being processed.

##### Your project #####
Options above, as well as:
`-DAEGIS_DYN_LINK` used when linking the library as a shared object</br>
`-DAEGIS_HEADER_ONLY` to make library header-only (default option)</br>
`-DAEGIS_SEPARATE_COMPILATION` used when linking the library as static or separate cpp file within your project</br>

## CMake misc ##
If configured with CMake, it will create a pkg-config file that may help with compiling your own project.</br>
It can be used as such:</br>
`g++ -std=c++14 myfile.cpp $(pkg-config --cflags --libs aegis)`</br>
to link to the shared object

`g++ -std=c++14 minimal.cpp $(pkg-config --cflags --libs aegis_static)`</br>
to link to the static object</br>

You can also use this library within your own CMake project by adding `find_package(Aegis REQUIRED)` to your `CMakeLists.txt`.


## Config ##
You can change basic configuration options within the `config.json` file that should be in the same directory as the executable when built.
```
{
	"token": "BOTTOKENHERE",
	"force-shard-count": 10
}
```
