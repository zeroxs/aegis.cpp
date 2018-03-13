//
// snowflake.hpp
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


namespace aegiscpp
{

/**\todo Needs documentation
*/
class snowflake
{
public:
    snowflake() : snowflake_id(0) {}
    snowflake(uint64_t _snowflake) : snowflake_id(_snowflake) {}

    operator uint64_t() const
    {
        return snowflake_id;
    }

    constexpr std::tuple<uint64_t, uint8_t, uint8_t, uint16_t> get_all()
    {
        return { get_timestamp(), get_worker(), get_process(), get_count() };
    };

    constexpr uint16_t get_count()
    {
        return static_cast<int16_t>(snowflake_id & _countMask);
    };

    constexpr uint8_t get_process()
    {
        return static_cast<int8_t>(snowflake_id & _workerMask) >> 12;
    };

    constexpr uint8_t get_worker()
    {
        return static_cast<int8_t>(snowflake_id & _workerMask) >> 17;
    };

    constexpr uint64_t get_timestamp()
    {
        return (snowflake_id & _timestampMask) >> 22;
    };

    static constexpr std::tuple<uint64_t, uint8_t, uint8_t, uint16_t> c_get_all(uint64_t snowflake)
    {
        return { c_get_timestamp(snowflake), c_get_worker(snowflake), c_get_process(snowflake), c_get_count(snowflake) };
    };

    static constexpr uint16_t c_get_count(uint64_t snowflake)
    {
        return static_cast<int16_t>(snowflake & _countMask);
    };

    static constexpr int8_t c_get_process(uint64_t snowflake)
    {
        return static_cast<int8_t>(snowflake & _workerMask) >> 12;
    };

    static constexpr uint8_t c_get_worker(uint64_t snowflake)
    {
        return static_cast<int8_t>(snowflake & _workerMask) >> 17;
    };

    static constexpr uint64_t c_get_timestamp(uint64_t snowflake)
    {
        return (snowflake & _timestampMask) >> 22;
    };

    constexpr uint64_t get_time()
    {
        return get_timestamp() + _discordEpoch;
    };

    static constexpr uint64_t c_get_time(uint64_t snowflake)
    {
        return c_get_timestamp(snowflake) + _discordEpoch;
    };

private:
    uint64_t snowflake_id;
    static constexpr uint64_t _countMask = 0x0000000000000FFFL;
    static constexpr uint64_t _processMask = 0x000000000001F000L;
    static constexpr uint64_t _workerMask = 0x00000000003E0000L;
    static constexpr uint64_t _timestampMask = 0xFFFFFFFFFFC00000L;
    static constexpr uint64_t _discordEpoch = 1420070400000;
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, snowflake& s)
{
    if (j.is_string())
        s = std::stoll(j.get<std::string>());
    else if (j.is_number())
        s = j.get<int64_t>();
}

/**\todo Needs documentation
*/
inline void to_json(nlohmann::json& j, const snowflake& s)
{
    j = json{ static_cast<int64_t>(s) };
}

}

