//
// utility.hpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <string>
#include <atomic>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
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

/**\todo Needs documentation
 */
enum class bot_status
{
    Uninitialized = 0,
    Running,
    Shutdown
};

enum class shard_status
{
    Uninitialized = 0,
    Connecting,
    PreReady,
    Online,
    Reconnecting,
    Closed,
    Shutdown
};

namespace utility
{

template<class Func>
std::string perf_run(const std::string & name, Func f)
{
    std::stringstream ss;
    ss << "Running [" << name << "]\n";
    auto n = std::chrono::steady_clock::now();

    f();

    auto n_end = std::chrono::steady_clock::now();

    ss << "Time: [" << std::chrono::duration_cast<std::chrono::microseconds>(n_end - n).count() << "us]\n";
    return ss.str();
}

/// Returns the duration of the time since epoch for the timer.
/**
 * @param t The time_point to convert to the provided duration template parameter
 * @returns std::chrono::duration of the time since epoch start until the provided time_point
 */
template<typename Duration, typename T>
inline Duration to_t(const T & t)
{
    return std::chrono::duration_cast<Duration>(t.time_since_epoch());
}

/// Returns the duration of the time since epoch for the timer.
/// Specialized template for conversions from nanoseconds.
/**
 * @param t The time_point to convert to the provided duration template parameter
 * @returns std::chrono::duration of the time since epoch start until the provided time_point
 */
template<typename Duration>
inline Duration to_t(const std::chrono::duration<int64_t, std::nano> & t)
{
    return std::chrono::duration_cast<Duration>(t);
}

/// Returns the value of the time since epoch for the timer.
/// For steady_clock this is last reboot.
/// For system_clock this is the Unix epoch.
/**
 * @param t The time_point to convert to milliseconds
 * @returns int64_t of the time since epoch start until the provided time_point
 */
template<typename T>
inline int64_t to_ms(const T & t)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
}

/// Returns the value of the time since epoch for the timer.
/// Specialized template for nanoseconds.
/// For steady_clock this is last reboot.
/// For system_clock this is the Unix epoch.
/**
 * @param t The time_point to convert to milliseconds
 * @returns int64_t of the time since epoch start until the provided time_point
 */
template<>
inline int64_t to_ms(const std::chrono::duration<int64_t, std::nano> & t)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
}

/// Converts an ISO8601 date string to an std::chrono::system_clock::time_point
/// or other provided template parameter type
/**
 * @param _time_t String of the timestamp
 * @returns std::chrono::system_clock::time_point String timestamp converted to time_point
 */
template<typename T = std::chrono::system_clock::time_point>
T from_iso8601(const std::string & _time_t)
{
    std::tm tm = {};
    std::istringstream ss(_time_t);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

namespace platform
{

/**\todo Needs documentation
 */
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

#if defined(__clang__) || defined(__GNUC__)
# if (__cplusplus >= 201703)
#  define CXX_VERSION 17
# else
#  define CXX_VERSION 14
# endif
#elif defined(AEGIS_MSVC)
# if _MSVC_LANG == 201703L ||  _HAS_CXX17 == 1
#  define CXX_VERSION 17
# else
#  define CXX_VERSION 14
# endif
#endif


#if defined(_WIN64)
constexpr const char * m_platform = "Windows x64";
constexpr OS m_os = OS::Windows64;
#elif defined(_WIN32)
constexpr const char * m_platform = "Windows x86";
constexpr OS m_os = OS::Windows32;
#elif defined(__CYGWIN__) && !defined(_WIN32)
constexpr const char * m_platform = "Windows (Cygwin)";
constexpr OS m_os = OS::undefined;
#elif defined(__linux__)
constexpr const char * m_platform = "Linux";
constexpr OS m_os = OS::Linux;
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
#include <sys/param.h>
#if defined(BSD)
constexpr const char * m_platform = "*BSD";
constexpr OS m_os = OS::BSD;
#endif
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
#include <TargetConditionals.h>
constexpr const char * m_platform = "OSX";
constexpr OS m_os = OS::Mac;
#else
constexpr const char * m_platform = "undefined";
constexpr OS m_os = OS::undefined;
#endif


/**\todo Needs documentation
 */
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

}

}
