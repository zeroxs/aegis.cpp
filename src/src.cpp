//
// src.cpp
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

#define AEGIS_SOURCE

#include <aegis/config.hpp>

#if defined(AEGIS_HEADER_ONLY)
# error Do not compile Aegis library source with AEGIS_HEADER_ONLY defined
#endif

#include <zstr.hpp>

#include <aegis/error.hpp>

#include <aegis/config.hpp>
#include <aegis/common.hpp>
#include <aegis/utility.hpp>

#include <aegis/snowflake.hpp>
#include <aegis/role.hpp>
#include <aegis/error.hpp>

#include <aegis/aegis.cpp>
#include <aegis/guild.cpp>
#include <aegis/channel.cpp>
#include <aegis/shard.cpp>
#include <aegis/member.cpp>
#include <aegis/ratelimit.cpp>
