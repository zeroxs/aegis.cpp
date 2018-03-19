//
// message_create.hpp
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
#include "../error.hpp"
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
struct message_create
{
    bool has_member() { return _member; }
    bool has_channel() { return _channel; }
    member & get_member() { if (_member == nullptr) throw aegiscpp::exception("Member not set", make_error_code(error::member_not_found)); return *_member; }
    channel & get_channel() { if (_channel == nullptr) throw aegiscpp::exception("Channel not set", make_error_code(error::channel_not_found)); return *_channel; }
    message msg; /**<\todo Needs documentation */
    shard * const _shard; /**<\todo Needs documentation */
    aegis * const bot; /**<\todo Needs documentation */
    channel * const _channel; /**<\todo Needs documentation */
    member * const _member; /**<\todo Needs documentation */
};

}

