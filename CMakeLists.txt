message(STATUS "CMake version: ${CMAKE_VERSION}")

cmake_minimum_required(VERSION 3.8)

file(READ ${CMAKE_SOURCE_DIR}/include/aegis/version.hpp version_hpp)
if (NOT version_hpp MATCHES "AEGIS_VERSION_SHORT ([0-9][0-9])([0-9][0-9])([0-9][0-9])")
  message(FATAL_ERROR "Cannot get AEGIS_VERSION_SHORT from version.hpp. ${CMAKE_SOURCE_DIR}/include/aegis/version.hpp")
endif ()
math(EXPR AEGIS_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR AEGIS_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR AEGIS_VERSION_PATCH ${CMAKE_MATCH_3})
string(CONCAT AEGIS_VERSION ${AEGIS_VERSION_MAJOR}.${AEGIS_VERSION_MINOR}.${AEGIS_VERSION_PATCH})


project(libaegis VERSION ${AEGIS_VERSION} LANGUAGES CXX)


set(CMAKE_INCLUDE_PATH
${CMAKE_SOURCE_DIR}/lib/asio/asio/include
${CMAKE_SOURCE_DIR}/lib/zstr/src
${CMAKE_SOURCE_DIR}/lib/websocketpp
${CMAKE_SOURCE_DIR}/lib/spdlog
${CMAKE_SOURCE_DIR}/lib/json/single_include)


list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Asio 1.12.0 REQUIRED MODULE)
find_package(ZStr REQUIRED MODULE)
find_package(JSON 3.1.2 REQUIRED MODULE)
find_package(Spdlog 0.16.4 REQUIRED MODULE)
find_package(Websocketpp 0.7.0 REQUIRED MODULE)
find_package(OpenSSL 1.0.1 REQUIRED MODULE)
find_package(ZLIB REQUIRED MODULE)

option(BUILD_STATIC_LIBS "Build the static library" ON)
option(BUILD_SHARED_LIBS "Build the shared library" ON)
option(BUILD_EXAMPLES "Build example programs" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_BUILD_TYPE Release)

set(AEGIS_FILES
include/aegis/channel.cpp
include/aegis/core.cpp
include/aegis/guild.cpp
include/aegis/member.cpp
include/aegis/ratelimit.cpp
include/aegis/rest_controller.cpp
include/aegis/shard.cpp)

add_library(aegis_obj OBJECT ${AEGIS_FILES})

add_library(aegis $<TARGET_OBJECTS:aegis_obj>)
add_library(aegis_static STATIC $<TARGET_OBJECTS:aegis_obj>)
add_library(Aegis::aegis ALIAS aegis)

set_target_properties(aegis aegis_obj PROPERTIES POSITION_INDEPENDENT_CODE 1)

set_target_properties(aegis_obj PROPERTIES CXX_STANDARD 14)
set_target_properties(aegis_obj PROPERTIES CXX_STANDARD_REQUIRED ON)

set_target_properties(aegis PROPERTIES SOVERSION 1)
set_target_properties(aegis PROPERTIES CXX_STANDARD 14)
set_target_properties(aegis PROPERTIES CXX_STANDARD_REQUIRED ON)

set_target_properties(aegis_static PROPERTIES OUTPUT_NAME "aegis")
set_target_properties(aegis_static PROPERTIES CXX_STANDARD 14)
set_target_properties(aegis_static PROPERTIES CXX_STANDARD_REQUIRED ON)

target_compile_definitions(aegis_obj PRIVATE -DAEGIS_SEPARATE_COMPILATION)
target_compile_definitions(aegis PRIVATE -DAEGIS_DYN_LINK)
target_compile_definitions(aegis_static PRIVATE -DAEGIS_SEPARATE_COMPILATION)

target_link_libraries(aegis PUBLIC ssl crypto z dl -pthread)

#HINTS ${CMAKE_SOURCE_DIR}/lib
target_include_directories(aegis
  PUBLIC
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${Asio_INCLUDE_DIR}
    ${ZStr_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
    ${Spdlog_INCLUDE_DIR}
    ${Websocketpp_INCLUDE_DIR}
    ${OpenSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(aegis_obj
  PUBLIC
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${Asio_INCLUDE_DIR}
    ${ZStr_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
    ${Spdlog_INCLUDE_DIR}
    ${Websocketpp_INCLUDE_DIR}
    ${OpenSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if ((CXX_STANDARD EQUAL "17") OR (CMAKE_CXX_STANDARD EQUAL "17"))
target_compile_features(aegis_obj PRIVATE cxx_std_17)
target_compile_features(aegis_static PRIVATE cxx_std_17)
target_compile_features(aegis PRIVATE cxx_std_17)
else ()
target_compile_features(aegis_obj PRIVATE cxx_std_14)
target_compile_features(aegis_static PRIVATE cxx_std_14)
target_compile_features(aegis PRIVATE cxx_std_14)
endif ()

target_compile_options(aegis_obj PRIVATE -Wall -Wno-sign-compare -Wno-unknown-pragmas -Wno-unused-variable -Wno-noexcept-type -Wno-unused-result)
target_compile_options(aegis PRIVATE -Wall -Wno-sign-compare -Wno-unknown-pragmas -Wno-unused-variable -Wno-noexcept-type -Wno-unused-result)

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Aegis)

set_target_properties(aegis PROPERTIES EXPORT_NAME Aegis)
SET(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,--no-undefined")

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT aegis-targets
  FILE
    AegisTargets.cmake
  NAMESPACE
    Aegis::
  DESTINATION
    ${INSTALL_CONFIGDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/AegisConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/AegisConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AegisConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AegisConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AegisConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

target_include_directories(aegis_static
  PUBLIC
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${Asio_INCLUDE_DIR}
    ${ZStr_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
    ${Spdlog_INCLUDE_DIR}
    ${Websocketpp_INCLUDE_DIR}
    ${OpenSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(aegis_static PRIVATE -Wall -Wno-sign-compare -Wno-unknown-pragmas -Wno-unused-variable -Wno-noexcept-type -Wno-unused-result)
install(TARGETS aegis aegis_static
    EXPORT aegis-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
 
#SET(PKG_CONFIG_REQUIRES )
SET(PKG_CONFIG_LIBDIR
    "\${prefix}/lib"
)
SET(PKG_CONFIG_INCLUDEDIR
    "\${prefix}/include"
)
SET(PKG_CONFIG_LIBS
    "-L\${libdir} -laegis -lssl -lcrypto -pthread -lz -ldl"
)
SET(PKG_CONFIG_STATIC_LIBS
    "-L\${libdir} -l:libaegis.a -lssl -lcrypto -pthread -lz -ldl"
)
SET(PKG_CONFIG_CFLAGS
    "-I/usr/include -I\${includedir} -I${Asio_INCLUDE_DIR} -I${ZStr_INCLUDE_DIR} -I${JSON_INCLUDE_DIR} -I${Spdlog_INCLUDE_DIR} -I${Websocketpp_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIR} -DAEGIS_DYN_LINK"
)
SET(PKG_CONFIG_STATIC_CFLAGS
    "-I/usr/include -I\${includedir} -I${Asio_INCLUDE_DIR} -I${ZStr_INCLUDE_DIR} -I${JSON_INCLUDE_DIR} -I${Spdlog_INCLUDE_DIR} -I${Websocketpp_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIR} -DAEGIS_SEPARATE_COMPILATION"
)
   
CONFIGURE_FILE(
  "${CMAKE_SOURCE_DIR}/cmake/aegis_static.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/aegis_static.pc"
)
CONFIGURE_FILE(
  "${CMAKE_SOURCE_DIR}/cmake/aegis.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc"
)

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc" "${CMAKE_CURRENT_BINARY_DIR}/aegis_static.pc" DESTINATION lib/pkgconfig)

export(EXPORT aegis-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/AegisTargets.cmake NAMESPACE Aegis::)

export(PACKAGE Aegis)


if (BUILD_EXAMPLES)

add_executable(aegis_example src/example_main.cpp src/example.cpp)
target_compile_definitions(aegis_example PRIVATE -DAEGIS_DYN_LINK)

add_executable(aegis_minimal src/minimal.cpp)
target_compile_definitions(aegis_minimal PRIVATE -DAEGIS_DYN_LINK)

add_executable(aegis_headeronly_no_cache src/minimal.cpp)
target_compile_definitions(aegis_headeronly_no_cache PRIVATE -DAEGIS_DISABLE_ALL_CACHE)

set_property(TARGET aegis_example PROPERTY CXX_STANDARD 14)
set_property(TARGET aegis_example PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET aegis_minimal PROPERTY CXX_STANDARD 14)
set_property(TARGET aegis_minimal PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET aegis_headeronly_no_cache PROPERTY CXX_STANDARD 14)
set_property(TARGET aegis_headeronly_no_cache PROPERTY CXX_STANDARD_REQUIRED ON)


target_link_libraries(aegis_example PUBLIC aegis ssl crypto z dl Threads::Threads)
target_link_libraries(aegis_minimal PUBLIC aegis ssl crypto z dl Threads::Threads)
target_link_libraries(aegis_headeronly_no_cache PUBLIC aegis ssl crypto z dl Threads::Threads)

target_include_directories(aegis_example
  PUBLIC
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${Asio_INCLUDE_DIR}
    ${ZStr_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
    ${Spdlog_INCLUDE_DIR}
    ${Websocketpp_INCLUDE_DIR}
    ${OpenSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(aegis_minimal
  PUBLIC
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${Asio_INCLUDE_DIR}
    ${ZStr_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
    ${Spdlog_INCLUDE_DIR}
    ${Websocketpp_INCLUDE_DIR}
    ${OpenSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(aegis_headeronly_no_cache
  PUBLIC
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${Asio_INCLUDE_DIR}
    ${ZStr_INCLUDE_DIR}
    ${JSON_INCLUDE_DIR}
    ${Spdlog_INCLUDE_DIR}
    ${Websocketpp_INCLUDE_DIR}
    ${OpenSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


endif ()
