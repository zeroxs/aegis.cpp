//
// config.hpp
// **********
//
// Copyright (c) 2021 Sharon Fox (sharon at sharonfox dot dev)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <windows.h>
#endif

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
#  error aegis.cpp requires C++17 or greater
# endif
#else
# if (__cplusplus < 201703)
#  error aegis.cpp requires C++17 or greater
# endif
#endif
