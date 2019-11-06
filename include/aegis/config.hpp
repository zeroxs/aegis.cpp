//
// config.hpp
// **********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/version.hpp"
#include "aegis/fwd.hpp"

#if !defined(ASIO_NO_DEPRECATED)
#define ASIO_NO_DEPRECATED
#endif

#if !defined(ASIO_STANDALONE)
#define ASIO_STANDALONE
#endif

#if !defined(ASIO_HEADER_ONLY) && !defined(ASIO_DYN_LINK) && !defined(ASIO_SEPARATE_COMPILATION)
#define ASIO_HEADER_ONLY
#endif

#if !defined(_WEBSOCKETPP_CPP11_STL_)
#define _WEBSOCKETPP_CPP11_STL_
#endif

// Shamelessly make use of ASIO's config-style

// Default to a header-only implementation. The user must specifically request
// separate compilation by defining either AEGIS_SEPARATE_COMPILATION or
// AEGIS_DYN_LINK (as a DLL/shared library implies separate compilation).
#if !defined(AEGIS_HEADER_ONLY)
# if !defined(AEGIS_SEPARATE_COMPILATION)
#  if !defined(AEGIS_DYN_LINK)
#   define AEGIS_HEADER_ONLY 1
#  endif // !defined(AEGIS_DYN_LINK)
# endif // !defined(AEGIS_SEPARATE_COMPILATION)
#endif // !defined(AEGIS_HEADER_ONLY)

#if defined(AEGIS_HEADER_ONLY)
# define AEGIS_DECL inline
#else // defined(AEGIS_HEADER_ONLY)
# if defined(_MSC_VER)
// We need to import/export our code only if the user has specifically asked
// for it by defining AEGIS_DYN_LINK.
#  if defined(AEGIS_DYN_LINK)
// Export if this is our own source, otherwise import.
#   if defined(AEGIS_SOURCE)
#    define AEGIS_DECL __declspec(dllexport)
#   else // defined(AEGIS_SOURCE)
#    define AEGIS_DECL __declspec(dllimport)
#   endif // defined(AEGIS_SOURCE)
#  endif // defined(AEGIS_DYN_LINK)
# endif // defined(_MSC_VER)
#endif // defined(AEGIS_HEADER_ONLY)

// If AEGIS_DECL isn't defined yet define it now.
#if !defined(AEGIS_DECL)
# define AEGIS_DECL
#endif // !defined(AEGIS_DECL)

// Microsoft Visual C++ detection.
#if !defined(AEGIS_MSVC)
# if defined(_MSC_VER) && (defined(__INTELLISENSE__) \
      || (!defined(__MWERKS__) && !defined(__EDG_VERSION__)))
#  define AEGIS_MSVC _MSC_VER
# endif // defined(_MSC_VER) && defined(__INTELLISENSE__)
        //|| (!defined(__MWERKS__) && !defined(__EDG_VERSION__)))
#endif // defined(AEGIS_MSVC)
#if defined(AEGIS_MSVC)
# include <ciso646> // Needed for _HAS_CXX17.
#endif // defined(AEGIS_MSVC)

// Workaround for Microsoft Visual C++ __cplusplus value
#if defined(AEGIS_MSVC)
# if (_MSVC_LANG < 201402)
#  error AegisLib requires C++14 or greater
# endif
#else
# if (__cplusplus < 201402)
#  error AegisLib requires C++14 or greater
# endif
#endif

#if (__cplusplus >= 201703) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703) || (defined(_HAS_CXX17) && _HAS_CXX17 != 0)
# define AEGIS_CXX17
#endif // (__cplusplus >= 201703) || (_MSVC_LANG >= 201703)

// Support for std::optional over built-in
#if !defined(AEGIS_HAS_STD_OPTIONAL)
# if (__cplusplus >= 201703)
#  if __has_include(<optional>)
#   include <optional>
namespace aegis::lib
{
template<typename T> using optional = std::optional<T>;
constexpr auto nullopt = std::nullopt;
using bad_optional_access = std::bad_optional_access;
}
#   define AEGIS_HAS_STD_OPTIONAL 1
#  elif __has_include(<experimental/optional>)
#   include <experimental/optional>
namespace aegis::lib
{
template<typename T> using optional = std::experimental::optional<T>;
constexpr auto nullopt = std::experimental::nullopt;
using bad_optional_access = std::experimental::bad_optional_access;
}
#   define AEGIS_HAS_STD_OPTIONAL 1
#  endif // __has_include(<optional>)
# elif (__cplusplus >= 201402) // c++14
#  include "aegis/optional.hpp"
namespace aegis
{
namespace lib
{
template<typename T> using optional = std::experimental::optional<T>;
constexpr auto nullopt = std::experimental::nullopt;
using bad_optional_access = std::experimental::bad_optional_access;
}
}
#  define AEGIS_HAS_STD_OPTIONAL 1
#  define AEGIS_HAS_BUILTIN_OPTIONAL 1
# endif // (__cplusplus >= 201703)
# if defined(AEGIS_MSVC)
#  if (_MSVC_LANG < 201703)
#   include "aegis/optional.hpp"
namespace aegis
{
namespace lib
{
template<typename T> using optional = std::experimental::optional<T>;
constexpr auto nullopt = std::experimental::nullopt;
using bad_optional_access = std::experimental::bad_optional_access;
}
}
#   define AEGIS_HAS_STD_OPTIONAL 1
#   define AEGIS_HAS_BUILTIN_OPTIONAL 1
#  else
#   include <optional>
namespace aegis::lib
{
template<typename T> using optional = std::optional<T>;
constexpr auto nullopt = std::nullopt;
using bad_optional_access = std::bad_optional_access;
}
#   define AEGIS_HAS_STD_OPTIONAL 1
#  endif // (_MSC_VER >= 1910 && _HAS_CXX17)
# endif // defined(AEGIS_MSVC)
#endif // !defined(AEGIS_HAS_STD_OPTIONAL)

#if !defined(AEGIS_HAS_STD_OPTIONAL)
# error Could not find a suitable optional library.
#endif

// use std::shared_timed_mutex on C++14 or shared_mutex on C++17
#if !defined(AEGIS_HAS_STD_SHARED_MUTEX)
# if !defined(AEGIS_DISABLE_STD_SHARED_MUTEX)
#  if (__cplusplus >= 201703) || (_MSVC_LANG >= 201703)
#   define AEGIS_HAS_STD_SHARED_MUTEX 1
#  endif // (__cplusplus >= 201703) || (_MSVC_LANG >= 201703)
# endif // !defined(AEGIS_DISABLE_STD_SHARED_MUTEX)
# if !defined(AEGIS_HAS_STD_SHARED_MUTEX)
#  define AEGIS_HAS_STD_SHARED_MUTEX 0
# endif // !defined(AEGIS_HAS_STD_SHARED_MUTEX)
#endif // !defined(AEGIS_HAS_STD_SHARED_MUTEX)

#if !defined(NDEBUG)
#include <cassert>
#endif

/// spdlog defines
#if defined(DEBUG) || defined(_DEBUG)
#define AEGIS_DEBUG(logger, ...) if (logger->should_log(spdlog::level::level_enum::debug)) logger->debug(__VA_ARGS__)
#define SPDLOG_STR_H(x) #x
#define SPDLOG_STR_HELPER(x) SPDLOG_STR_H(x)
#ifdef AEGIS_MSVC
#define AEGIS_TRACE(logger, ...) if (logger->should_log(spdlog::level::level_enum::trace)) logger->trace("[ " __FILE__ "(" SPDLOG_STR_HELPER(__LINE__) ") ] " __VA_ARGS__)
#else
#define AEGIS_TRACE(logger, ...) if (logger->should_log(spdlog::level::level_enum::trace)) logger->trace("[ " __FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " ] " __VA_ARGS__)
#endif

#else
#define AEGIS_DEBUG(logger, ...) (void)0
#define AEGIS_TRACE(logger, ...) (void)0
#endif

#if defined(AEGIS_CONTEXT_ROUNDROBIN) || defined(AEGIS_CONTEXT_MODULUS) || defined(AEGIS_CONTEXT_LOADBALANCE)
#define AEGIS_CONTEXT
#endif
