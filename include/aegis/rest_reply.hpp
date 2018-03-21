//
// rest_reply.hpp
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


#include "aegis/config.hpp"
#include <websocketpp/http/constants.hpp>
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <string>


namespace aegiscpp
{

/**\todo Needs documentation
*/
struct rest_reply
{
    rest_reply() = default;
    rest_reply(bool p) { permissions = p; }
    rest_reply(websocketpp::http::status_code::value reply_code, bool global, int32_t limit, int32_t remaining, int64_t reset, int32_t retry, std::string content)
        : reply_code(reply_code)
        , global(global)
        , limit(limit)
        , remaining(remaining)
        , reset(reset)
        , retry(retry)
        , content(content)
    {
    }
    websocketpp::http::status_code::value reply_code; /**< REST HTTP reply code */
    bool global = false; /**< Is global ratelimited */
    int32_t limit = 0; /**< Rate limit current endpoint call limit */
    int32_t remaining = 0; /**< Rate limit remaining count */
    int64_t reset = 0; /**< Rate limit reset time */
    int32_t retry = 0; /**< Rate limit retry time */
    std::string content; /**< REST call's reply body */
    bool permissions = true; /**< Whether the call had proper permissions */
};

}

