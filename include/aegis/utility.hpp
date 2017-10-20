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



#include "aegis/config.hpp"
#include <websocketpp/common/random.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/roles/client_endpoint.hpp>
#include <websocketpp/client.hpp>


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

template <typename F, typename... Ts>
inline auto async(F&& f, Ts&&... params)
{
    return std::async(std::launch::async, std::forward<F>(f),
                      std::forward<Ts>(params)...);
}

namespace utility
{


#define OPTION(x)\
template <typename C> static std::true_type test_##x(checker<C, decltype(&C::x)> *);\
template <typename C> static std::false_type test_##x(...);\
template <typename C> static constexpr bool getvalue_##x(checker<C, decltype(&C::x)> *) { return C::x; }\
template <typename C> static constexpr bool getvalue_##x(...) { return false; }\
\
template <typename C>\
static constexpr std::pair<const decltype(test_##x<C>(nullptr)), const bool> get_##x() { return { decltype(test_##x<T>(nullptr))(), getvalue_##x<T>(nullptr) }; }\
\
struct x\
{\
    typedef decltype(test_##x<T>(nullptr)) x##_t;\
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


} //aegis
