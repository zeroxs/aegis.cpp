//
// config.hpp
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

#pragma once


#define AEGIS_VERSION_MAJOR     0
#define AEGIS_VERSION_MINOR     1
#define AEGIS_VERSION_REVISION  0

#define ASIO_STANDALONE
#define ASIO_HAS_STD_ATOMIC
#define ASIO_HAS_STD_CHRONO
#define _WEBSOCKETPP_CPP11_STL_
#define BOOST_DATE_TIME_NO_LIB
#define BOOST_REGEX_NO_LIB

namespace aegis
{

struct settings
{
    static constexpr bool selfbot = false;
    static constexpr uint64_t owner_id = 171000788183678976;
    static constexpr uint32_t force_shard_count = 0;
};

enum state
{
    UNINITIALIZED = 0,
    READY = 1,
    CONNECTING = 2,
    ONLINE = 3,
    RECONNECTING = 4,
    SHUTDOWN = 5
};


}

