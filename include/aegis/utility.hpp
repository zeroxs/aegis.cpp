//
// utility.hpp
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

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <spdlog/spdlog.h>
#include <websocketpp/common/random.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/roles/client_endpoint.hpp>
#include <websocketpp/client.hpp>
#include <asio/steady_timer.hpp>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

namespace aegis
{
using namespace std::string_view_literals;

namespace utility
{


#define OPTION(x)\
template <typename C> static std::true_type test_##x(checker<C, decltype(&C::owner_id)> *);\
template <typename C> static std::false_type test_##x(...);\
template <typename C> static constexpr bool getvalue_##x(checker<C, decltype(&C::owner_id)> *) { return C::owner_id; }\
template <typename C> static constexpr bool getvalue_##x(...) { return false; }\
\
template <typename C>\
static constexpr std::pair<const decltype(test_##x<C>(nullptr)), const bool> get_##x() { return { decltype(test_##x<T>(nullptr))(), getvalue_##x<T>(nullptr) }; }\
\
struct x\
{\
    typedef decltype(test_##x<T>(nullptr)) option_t;\
    static constexpr bool test() { auto c = get_##x<T>(); return (std::get<0>(c) && std::get<1>(c)); }\
};


template <typename T>
struct check_setting
{
    template <class, class> class checker;

    OPTION(owner_id);
    OPTION(selfbot);
    OPTION(force_shard_count);
    OPTION(debugmode);
    OPTION(disable_cache);

//     template <typename C> static std::true_type test_owner_id(checker<C, decltype(&C::owner_id)> *);
//     template <typename C> static std::false_type test_owner_id(...);
//     template <typename C> static constexpr bool getvalue_owner_id(checker<C, decltype(&C::owner_id)> *) { return C::owner_id; }
//     template <typename C> static constexpr bool getvalue_owner_id(...) { return false; }
// 
//     template <typename C> static std::true_type test_selfbot(checker<C, decltype(&C::selfbot)> *);
//     template <typename C> static std::false_type test_selfbot(...);
//     template <typename C> static constexpr bool getvalue_selfbot(checker<C, decltype(&C::selfbot)> *) { return C::selfbot; }
//     template <typename C> static constexpr bool getvalue_selfbot(...) { return false; }
// 
//     template <typename C> static std::true_type test_force_shard_count(checker<C, decltype(&C::force_shard_count)> *);
//     template <typename C> static std::false_type test_force_shard_count(...);
//     template <typename C> static constexpr bool getvalue_force_shard_count(checker<C, decltype(&C::force_shard_count)> *) { return C::force_shard_count; }
//     template <typename C> static constexpr bool getvalue_force_shard_count(...) { return false; }
// 
//     template <typename C> static std::true_type test_debugmode(checker<C, decltype(&C::force_shard_count)> *);
//     template <typename C> static std::false_type test_debugmode(...);
//     template <typename C> static constexpr bool getvalue_debugmode(checker<C, decltype(&C::force_shard_count)> *) { return C::force_shard_count; }
//     template <typename C> static constexpr bool getvalue_debugmode(...) { return false; }
// 
//     template <typename C> static std::true_type test_disable_cache(checker<C, decltype(&C::force_shard_count)> *);
//     template <typename C> static std::false_type test_disable_cache(...);
//     template <typename C> static constexpr bool getvalue_disable_cache(checker<C, decltype(&C::force_shard_count)> *) { return C::force_shard_count; }
//     template <typename C> static constexpr bool getvalue_disable_cache(...) { return false; }
// 
//     template <typename C>
//     static constexpr std::pair<const decltype(test_owner_id<C>(nullptr)), const bool> get_owner_id() { return { decltype(test_owner_id<T>(nullptr))(), getvalue_owner_id<T>(nullptr) }; }
//     template <typename C>
//     static constexpr std::pair<const decltype(test_selfbot<C>(nullptr)), const bool> get_selfbot() { return { decltype(test_selfbot<T>(nullptr))(), getvalue_selfbot<T>(nullptr) }; }
//     template <typename C>
//     static constexpr std::pair<const decltype(test_force_shard_count<C>(nullptr)), const bool> get_force_shard_count() { return { decltype(test_force_shard_count<T>(nullptr))(), getvalue_force_shard_count<T>(nullptr) }; }
//     template <typename C>
//     static constexpr std::pair<const decltype(test_debugmode<C>(nullptr)), const bool> get_debugmode() { return { decltype(test_debugmode<T>(nullptr))(), getvalue_debugmode<T>(nullptr) }; }
//     template <typename C>
//     static constexpr std::pair<const decltype(test_disable_cache<C>(nullptr)), const bool> get_disable_cache() { return { decltype(test_disable_cache<T>(nullptr))(), getvalue_disable_cache<T>(nullptr) }; }
// 
//     struct disable_cache
//     {
//         typedef decltype(test_disable_cache<T>(nullptr)) option_t;
//         static constexpr bool test() { auto c = get_disable_cache<T>(); return (std::get<0>(c) && std::get<1>(c)); }
//     };
//     struct force_shard_count
//     {
//         typedef decltype(test_force_shard_count<T>(nullptr)) option_t;
//         static constexpr bool test() { auto c = get_force_shard_count<T>(); return (std::get<0>(c) && std::get<1>(c)); }
//     };
//     struct owner_id
//     {
//         typedef decltype(test_owner_id<T>(nullptr)) option_t;
//         static constexpr bool test() { auto c = get_owner_id<T>(); return (std::get<0>(c) && std::get<1>(c)); }
//     };
//     struct selfbot
//     {
//         typedef decltype(test_selfbot<T>(nullptr)) option_t;
//         static constexpr bool test() { auto c = get_selfbot<T>(); return (std::get<0>(c) && std::get<1>(c)); }
//     };
//     struct debugmode
//     {
//         typedef decltype(test_debugmode<T>(nullptr)) option_t;
//         static constexpr bool test() { auto c = get_debugmode<T>(); return (std::get<0>(c) && std::get<1>(c)); }
//     };
};

namespace platform
{

enum class OS
{
    Linux,
    Windows32,
    Windows64,
    BSD,
    iOS,
    OSX,
    undefined
};

#if defined(_WIN64)
constexpr std::string_view m_platform = "Windows x64"sv;
constexpr OS m_os = OS::Windows64;
#elif defined(_WIN32)
constexpr std::string_view m_platform = "Windows x86"sv;
constexpr OS m_os = OS::Windows32;
#elif defined(__CYGWIN__) && !defined(_WIN32)
const std::string_view m_platform = "Windows (Cygwin)"sv;
constexpr OS m_os = OS::undefined;
#elif defined(__linux__)
constexpr std::string_view m_platform = "Linux"sv;
constexpr OS m_os = OS::Linux;
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
#include <sys/param.h>
#if defined(BSD)
constexpr std::string_view m_platform = "*BSD"sv;
constexpr OS m_os = OS::BSD;
#endif
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
#include <TargetConditionals.h>
constexpr std::string_view m_platform = "OSX"sv;
constexpr OS m_os = OS::Mac;
#else
constexpr std::string_view m_platform = "undefined"sv;
constexpr OS m_os = OS::undefined;
#endif


inline const std::string get_platform()
{
    return std::string(m_platform);
};

} //platform

  //////////////////////////////////////////////////////////////////////////
  /*
  * Author:  David Robert Nadeau
  * Site:    http://NadeauSoftware.com/
  * License: Creative Commons Attribution 3.0 Unported License
  *          http://creativecommons.org/licenses/by/3.0/deed.en_US
  */
  /**
  * Returns the peak (maximum so far) resident set size (physical
  * memory use) measured in bytes, or zero if the value cannot be
  * determined on this OS.
  */
size_t getPeakRSS();
size_t getCurrentRSS();

inline size_t getPeakRSS()
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.PeakWorkingSetSize;

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#else
    return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L;			/* Unsupported. */
#endif
}

/**
* Returns the current resident set size (physical memory use) measured
* in bytes, or zero if the value cannot be determined on this OS.
*/
inline size_t getCurrentRSS()
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
        (task_info_t)&info, &infoCount) != KERN_SUCCESS)
        return (size_t)0L;		/* Can't access? */
    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE* fp = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL)
        return (size_t)0L;		/* Can't open? */
    if (fscanf(fp, "%*s%ld", &rss) != 1)
    {
        fclose(fp);
        return (size_t)0L;		/* Can't read? */
    }
    fclose(fp);
    return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L;			/* Unsupported. */
#endif
}
//////////////////////////////////////////////////////////////////////////

} //utility


namespace spd = spdlog;
using namespace std::literals;
using namespace std::chrono;
using json = nlohmann::json;

struct settings;

using utility::check_setting;


} //aegis
