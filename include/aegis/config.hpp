//
// config.hpp
// **********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/version.hpp"
#include "aegis/fwd.hpp"

#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <windows.h>
#endif

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
# if (_MSVC_LANG < 201703)
#  error AegisLib requires C++17 or greater
# endif
#else
# if (__cplusplus < 201703)
#  error AegisLib requires C++17 or greater
# endif
#endif

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
