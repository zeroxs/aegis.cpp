//
// message_update.hpp
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

#include "../config.hpp"
#include "../snowflake.hpp"
#include "../objects/message.hpp"
#include "base_event.hpp"
#include <string>
#include <vector>

namespace aegiscpp
{

class member;
class channel;
class shard;
class aegis;

/**\todo Needs documentation
*/
struct message_update : public base_event
{
    message_update(const json & j, channel * c, member * m) : msg(j), _channel(c), _member(m) {};
    message msg; /**<\todo Needs documentation */
    channel * const _channel; /**<\todo Needs documentation */
    member * const _member; /**<\todo Needs documentation */
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, message_update& m)
{
    m.msg = j;
}

}

