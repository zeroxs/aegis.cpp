//
// version.hpp
// ***********
//
// Copyright (c) 2021 Sharon Fox (sharon at sharonfox dot dev)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#if !defined(AEGIS_VERSION_MAJOR)

#define AEGIS_VERSION_MAJOR 0
#define AEGIS_VERSION_MINOR 0
#define AEGIS_VERSION_PATCH 1
#define AEGIS_VERSION_BUILD 2021/03/12

#define AEGIS_VERSION_LONG "0.0.1 2021/03/12"

#if (__cplusplus < 201703)
# error aegis.cpp requires C++17 or greater
#endif

#endif
