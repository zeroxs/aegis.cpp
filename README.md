[![Build Status](https://travis-ci.org/zeroxs/aegis.cpp.svg?branch=master)](https://travis-ci.org/zeroxs/aegis.cpp) [![Discord](https://discordapp.com/api/guilds/287048029524066334/widget.png)](https://discord.gg/w7Y3Bb8) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)


Aegis Library
=======

C++14/17 library for interfacing with the [Discord API](https://discordapp.com/developers/docs/intro)

# License #

This project is licensed under the MIT license. See [LICENSE](https://github.com/zeroxs/aegis.cpp/blob/master/LICENSE)

Libraries used (all are header-only with the exception of zlib and openssl):
- [Asio](https://github.com/chriskohlhoff/asio)
- [Websocketpp (unofficial fork)](https://github.com/zeroxs/websocketpp)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog) (by extension, [fmtlib](https://github.com/fmtlib/fmt))
- [OpenSSL 1.0.2](https://www.openssl.org)
- [zlib](https://zlib.net)
- [zstr](https://github.com/mateidavid/zstr)



# TODO #
- Voice data send/recv
- Finish documentation
- Finish live example of library in use

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
    bot.set_on_message_create([](auto obj)
    {
        if (obj.msg.get_content() == "Hi")
            obj.msg.get_channel().create_message(fmt::format("Hello {}", obj.msg.author.username));
    });
    bot.run();
    bot.yield();
}
```

#### Separate compilation ####
You can include `#include <aegis/src.hpp>` within a single cpp file while defining `-DAEGIS_SEPARATE_COMPILATION`, have `#include <aegis.hpp>` in your program, then build as usual.

#### Shared/Static library ####
You can build this library with CMake.
```
$ git clone --recursive https://github.com/zeroxs/aegis.cpp.git
$ cd aegis.cpp
$ mkdir build
$ cd build
$ cmake ..
// or to use C++17
$ cmake -DCMAKE_CXX_COMPILER=g++-7 -DCMAKE_CXX_STANDARD=17 ..
```
You can also add `-DBUILD_EXAMPLES=1` and it will build 3 examples within the ./src directory.<br />
`example_main.cpp;example.cpp` will build a bot that runs out of its own class<br />
`minimal.cpp` will build two versions, one (aegis_minimal) will be with the shared/static library. The other (aegis_headeronly_no_cache) will be header-only but the lib will store no internal cache.


## Compiler Options ##
You can pass these flags to CMake to change what it builds<br />
`-DBUILD_EXAMPLES=1` will build the examples<br />
`-DCMAKE_CXX_COMPILER=g++-7` will let you select the compiler used<br />
`-DCMAKE_CXX_STANDARD=17` will let you select C++14 (default) or C++17

##### Library #####
You can pass these flags to your compiler (and/or CMake) to alter how the library is built<br />
`-DAEGIS_DISABLE_ALL_CACHE` will disable the internal caching of most objects such as member data reducing memory usage by a significant amount<br />
`-DAEGIS_DEBUG_HISTORY` enables the saving of the last 5 messages sent on the shard's websocket. In the event of an uncaught exception, they are dumped to console.<br />
`-DAEGIS_PROFILING` enables the usage of 3 callbacks that can help track time spent within the library. See docs:<br />
1. `aegis::core::set_on_message_end` Called when message handler is finished. Counts only your message handler time.
2. `aegis::core::set_on_js_end` Called when the incoming json event is parsed. Counts only json parse time.
3. `aegis::core::set_on_rest_end` Called when a REST (or any HTTP request is made) is finished. Counts only entire HTTP request time and includes response status code.

##### Your project #####
Options above, as well as:
`-DAEGIS_DYN_LINK` used when linking the library as a shared object<br />
`-DAEGIS_HEADER_ONLY` to make library header-only (default option)<br />
`-DAEGIS_SEPARATE_COMPILATION` used when linking the library as static or separate cpp file within your project<br />

## CMake misc ##
If configured with CMake, it will create a pkg-config file that may help with compiling your own project.<br />
It can be used as such:<br />
`g++ -std=c++14 myfile.cpp $(pkg-config --cflags --libs aegis)`<br />
to link to the shared object

`g++ -std=c++14 myfile.cpp $(pkg-config --cflags --libs aegis_static)`<br />
to link to the static object<br />

You can also use this library within your own CMake project by adding `find_package(Aegis REQUIRED)` to your `CMakeLists.txt`.


## Config ##
You can change basic configuration options within the `config.json` file. It should be in the same directory as the executable.
```
{
	"token": "BOTTOKENHERE",
	"force-shard-count": 10,
	"file-logging": false,
	"log-format": "%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v"
}
```
