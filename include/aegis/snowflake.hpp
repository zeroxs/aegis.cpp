//
// snowflake.hpp
// *************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/fwd.hpp"
#include "aegis/config.hpp"
#include <nlohmann/json.hpp>


namespace aegis
{

/// Stores creation time and extra data specific to Discord for entities
class snowflake
{
public:
    constexpr snowflake() noexcept : _id(0) {}
    constexpr snowflake(int64_t _snowflake) noexcept : _id(_snowflake) {}
    constexpr snowflake(const snowflake & _snowflake) noexcept : _id(_snowflake._id) {}
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    explicit snowflake(const char * _snowflake) noexcept : _id(std::stoll(std::string(_snowflake))) {}
    explicit snowflake(const std::string & _snowflake) noexcept : _id(std::stoll(_snowflake)) {}
#if defined(AEGIS_CXX17)
    explicit snowflake(const std::string_view _snowflake) noexcept : _id(std::stoll(std::string{ _snowflake })) {}
#endif
    explicit snowflake(const nlohmann::json & _snowflake) noexcept : _id(std::stoll(_snowflake.get<std::string>())) {}
    AEGIS_DECL snowflake(const aegis::user & _user) noexcept;
    AEGIS_DECL snowflake(const aegis::guild & _guild) noexcept;
    AEGIS_DECL snowflake(const aegis::channel & _channel) noexcept;
    AEGIS_DECL snowflake(const aegis::gateway::objects::role & _role) noexcept;
    AEGIS_DECL snowflake(const aegis::gateway::objects::message & _message) noexcept;
    AEGIS_DECL snowflake(const aegis::gateway::objects::emoji & _emoji) noexcept;
    AEGIS_DECL snowflake(const aegis::gateway::objects::attachment & _attachment) noexcept;
#endif

    constexpr operator int64_t() const noexcept
    {
        return _id;
    }

    /// Get snowflake as int64_t
    /**
     * @returns int64_t Snowflake
     */
    constexpr int64_t get() const noexcept
    {
        return _id;
    };

    /// Get snowflake as std::string
    /**
     * @returns std::string Snowflake
     */
    std::string gets() const noexcept
    {
        return std::to_string(_id);
    };

    /// Obtain all the snowflake values as a tuple
    /**
     * @returns std::tuple of all the snowflake parts
     */
    constexpr std::tuple<int64_t, int8_t, int8_t, int16_t> get_all() const noexcept
    {
        return std::tuple<int64_t, int8_t, int8_t, int16_t>{ get_timestamp(), get_worker(), get_process(), get_count() };
    };

    /// 
    /**
     * @returns int16_t Internal counter for snowflakes
     */
    constexpr int16_t get_count() const noexcept
    {
        return static_cast<int16_t>(_id & _countMask);
    };

    /// 
    /**
     * @returns int8_t Process ID of the snowflake generator
     */
    constexpr int8_t get_process() const noexcept
    {
        return static_cast<int8_t>((_id & _workerMask) >> 12);
    };

    /// 
    /**
     * @returns int8_t Worker ID of the snowflake generator
     */
    constexpr int8_t get_worker() const noexcept
    {
        return static_cast<int8_t>((_id & _workerMask) >> 17);
    };

    /// 
    /**
     * @returns int64_t The Discord epoch timestamp this snowflake was generated
     */
    constexpr int64_t get_timestamp() const noexcept
    {
        return (_id & _timestampMask) >> 22;
    };

    /// Obtain all the snowflake values as a tuple
    /**
     * @returns std::tuple of all the snowflake parts
     */
    static constexpr std::tuple<int64_t, int8_t, int8_t, int16_t> c_get_all(int64_t _snowflake) noexcept
    {
        return std::tuple<int64_t, int8_t, int8_t, int16_t>{ c_get_timestamp(_snowflake), c_get_worker(_snowflake), c_get_process(_snowflake), c_get_count(_snowflake) };
    };

    /// 
    /**
     * @returns int16_t Internal counter for snowflakes
     */
    static constexpr int16_t c_get_count(int64_t _snowflake) noexcept
    {
        return static_cast<int16_t>(_snowflake & _countMask);
    };

    /// 
    /**
     * @returns int8_t Process ID of the snowflake generator
     */
    static constexpr int8_t c_get_process(int64_t _snowflake) noexcept
    {
        return static_cast<int8_t>((_snowflake & _workerMask) >> 12);
    };

    /// 
    /**
     * @returns int8_t Worker ID of the snowflake generator
     */
    static constexpr int8_t c_get_worker(int64_t _snowflake) noexcept
    {
        return static_cast<int8_t>((_snowflake & _workerMask) >> 17);
    };

    /// 
    /**
     * @returns int64_t The Discord epoch timestamp this snowflake was generated
     */
    static constexpr int64_t c_get_timestamp(int64_t _snowflake) noexcept
    {
        return (_snowflake & _timestampMask) >> 22;
    };

    /// 
    /**
     * @returns int64_t Unix timestamp this snowflake was generated
     */
    constexpr int64_t get_time() const noexcept
    {
        return get_timestamp() + _discordEpoch;
    };

    /// 
    /**
     * @returns int64_t Unix timestamp this snowflake was generated
     */
    static constexpr int64_t c_get_time(int64_t _snowflake) noexcept
    {
        return c_get_timestamp(_snowflake) + _discordEpoch;
    };

private:
    uint64_t _id;
    static constexpr uint64_t _countMask = 0x0000000000000FFFL;
    static constexpr uint64_t _processMask = 0x000000000001F000L;
    static constexpr uint64_t _workerMask = 0x00000000003E0000L;
    static constexpr uint64_t _timestampMask = 0xFFFFFFFFFFC00000L;
    static constexpr uint64_t _discordEpoch = 1420070400000;
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, snowflake& s);
AEGIS_DECL void to_json(nlohmann::json& j, const snowflake& s);
/// \endcond

}

/// \cond TEMPLATES
namespace std
{

template <>
struct hash<aegis::snowflake>
{
    std::size_t operator()(const aegis::snowflake& k) const
    {
        return hash<int64_t>()(k.get());
    }
};

}
/// \endcond

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/impl/snowflake.cpp"
#endif
