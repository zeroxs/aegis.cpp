//
// aegis_impl.hpp
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
#include <exception>
#include <string>
#include <utility>
#include <system_error>


namespace aegis
{
typedef std::pair<std::error_code, std::string> err_str_pair;

namespace error
{
enum value
{
    /// Catch-all library error
    general = 1,

    /// Token invalid
    invalid_token,

    /// Bot was in the wrong state for this operation
    invalid_state,

    /// REST did not return websocket gateway url
    get_gateway



};

class category : public std::error_category
{
public:
    category() {}

    char const * name() const noexcept
    {
        return "aegis";
    }

    std::string message(int value) const
    {
        switch (value)
        {
            case error::general:
                return "Generic error";
            case error::invalid_token:
                return "Token invalid";
            case error::invalid_state:
                return "Invalid state";
            default:
                return "Unknown";
        }
    }
};

inline const std::error_category & get_category()
{
    static category instance;
    return instance;
}

inline std::error_code make_error_code(error::value e)
{
    return std::error_code(static_cast<int>(e), get_category());
}

}
}

namespace aegis
{
class exception : public std::exception
{
public:
    exception(std::string const & msg, std::error_code ec = make_error_code(error::general))
        : m_msg(msg.empty() ? ec.message() : msg)
        , m_code(ec)
    {
    }

    explicit exception(std::error_code ec)
        : m_msg(ec.message())
        , m_code(ec)
    {
    }

    ~exception() throw() {}

    virtual char const * what() const throw()
    {
        return m_msg.c_str();
    }

    std::error_code code() const throw()
    {
        return m_code;
    }

    const std::string m_msg;
    std::error_code m_code;
};
}
