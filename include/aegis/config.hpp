//
// config.hpp
// **********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
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

#if !defined(_WEBSOCKETPP_CPP11_STL_)
#define _WEBSOCKETPP_CPP11_STL_
#endif

#if !defined(BOOST_DATE_TIME_NO_LIB) 
#define BOOST_DATE_TIME_NO_LIB 
#endif 
#if !defined(BOOST_REGEX_NO_LIB) 
#define BOOST_REGEX_NO_LIB 
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

// Support noexcept on compilers known to allow it.
#if !defined(AEGIS_NOEXCEPT)
# if !defined(AEGIS_DISABLE_NOEXCEPT)
#  if defined(__clang__)
#   if __has_feature(__cxx_noexcept__)
#    define AEGIS_NOEXCEPT noexcept(true)
#    define AEGIS_NOEXCEPT_OR_NOTHROW noexcept(true)
#   endif // __has_feature(__cxx_noexcept__)
#  elif defined(__GNUC__)
#   if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)) || (__GNUC__ > 4)
#    if defined(__GXX_EXPERIMENTAL_CXX0X__)
#      define AEGIS_NOEXCEPT noexcept(true)
#      define AEGIS_NOEXCEPT_OR_NOTHROW noexcept(true)
#    endif // defined(__GXX_EXPERIMENTAL_CXX0X__)
#   endif // ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)) || (__GNUC__ > 4)
#  elif defined(AEGIS_MSVC)
#   if (_MSC_VER >= 1900)
#    define AEGIS_NOEXCEPT noexcept(true)
#    define AEGIS_NOEXCEPT_OR_NOTHROW noexcept(true)
#   endif // (_MSC_VER >= 1900)
#  endif // defined(AEGIS_MSVC)
# endif // !defined(AEGIS_DISABLE_NOEXCEPT)
# if !defined(AEGIS_NOEXCEPT)
#  define AEGIS_NOEXCEPT
# endif // !defined(AEGIS_NOEXCEPT)
# if !defined(AEGIS_NOEXCEPT_OR_NOTHROW)
#  define AEGIS_NOEXCEPT_OR_NOTHROW throw()
# endif // !defined(AEGIS_NOEXCEPT_OR_NOTHROW)
#endif // !defined(AEGIS_NOEXCEPT)

// Support for std::optional over built-in
#if !defined(AEGIS_HAS_STD_OPTIONAL)
# if !defined(AEGIS_DISABLE_STD_OPTIONAL)
#  if defined(__clang__)
#   if (__cplusplus >= 201703)
#    if __has_include(<optional>)
#     define AEGIS_HAS_STD_OPTIONAL 1
#    endif // __has_include(<optional>)
#   endif // (__cplusplus >= 201703)
#  endif // defined(__clang__)
#  if defined(__GNUC__)
#   if (__GNUC__ >= 7)
#    if (__cplusplus >= 201703)
#     define AEGIS_HAS_STD_OPTIONAL 1
#    endif // (__cplusplus >= 201703)
#   endif // (__GNUC__ >= 7)
#  endif // defined(__GNUC__)
#  if defined(AEGIS_MSVC)
#   if (_MSC_VER >= 1910 && defined(_HAS_CXX17))
#    define AEGIS_HAS_STD_OPTIONAL
#   endif // (_MSC_VER >= 1910 && _HAS_CXX17)
#  endif // defined(AEGIS_MSVC)
# endif // !defined(AEGIS_DISABLE_STD_OPTIONAL)
#endif // !defined(AEGIS_HAS_STD_OPTIONAL)

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

#if (__cplusplus >= 201703) || (_MSVC_LANG >= 201703) || defined(_HAS_CXX17)
# define AEGIS_CXX17
#endif // (__cplusplus >= 201703) || (_MSVC_LANG >= 201703)

#if !defined(NDEBUG)
#include <cassert>
#endif
