//
// config.hpp
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


#if !defined(AEGIS_VERSION_LONG)
#define AEGIS_VERSION_LONG      0x00000500
#define AEGIS_VERSION_TEXT      "aegis.cpp 0.5.0 2018/03/30"

#define AEGIS_VERSION_MAJOR     ((AEGIS_VERSION_LONG & 0x00ff0000) >> 16)
#define AEGIS_VERSION_MINOR     ((AEGIS_VERSION_LONG & 0x0000ff00) >> 8)
#define AEGIS_VERSION_PATCH     (AEGIS_VERSION_LONG & 0x000000ff)
#endif

#if !defined(SSL_R_SHORT_READ)
#define SSL_R_SHORT_READ 219 //temporarily fixes Websocket++ and OpenSSL1.1.x conflicts
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
// ASIO_DYN_LINK (as a DLL/shared library implies separate compilation).
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
# if defined(ASIO_HAS_BOOST_CONFIG) && defined(BOOST_MSVC)
#  define ASIO_MSVC BOOST_MSVC
# elif defined(_MSC_VER) && (defined(__INTELLISENSE__) \
      || (!defined(__MWERKS__) && !defined(__EDG_VERSION__)))
#  define AEGIS_MSVC _MSC_VER
# endif // defined(AEGIS_HAS_BOOST_CONFIG) && defined(BOOST_MSVC)
#endif // defined(AEGIS_MSVC)
