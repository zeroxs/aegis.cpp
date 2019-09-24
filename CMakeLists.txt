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

if(MSVC)
    add_compile_options("/W3" "$<$<CONFIG:RELEASE>:/O2>")
else()
	set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,--no-undefined")
	set(AEGIS_CFLAGS -Wno-sign-compare -Wno-unknown-pragmas -Wno-unused-variable -Wno-noexcept-type -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-missing-field-initializers)
    add_compile_options("$<$<CONFIG:RELEASE>:-O2>")
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options("-stdlib=libc++")
    else()
        # nothing special for gcc at the moment
    endif()
endif()

project(libaegis VERSION ${AEGIS_VERSION} LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Asio 1.12.0 REQUIRED MODULE)
find_package(JSON 3.1.2 REQUIRED MODULE)
find_package(spdlog 1.4.0 REQUIRED CONFIG)
find_package(Websocketpp 0.7.0 REQUIRED MODULE)
find_package(OpenSSL 1.0.1 REQUIRED MODULE)
find_package(ZLIB REQUIRED MODULE)

set(REQUIRED_LIBS OpenSSL::SSL ZLIB::ZLIB spdlog::spdlog JSON::JSON Asio::Asio dl Threads::Threads)

option(BUILD_SHARED_LIBS "Build the shared library" ON)
option(BUILD_EXAMPLES "Build example programs" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_BUILD_TYPE Release)

set(AEGIS_FILES
include/aegis/impl/channel.cpp
include/aegis/impl/core.cpp
include/aegis/impl/guild.cpp
include/aegis/impl/user.cpp
include/aegis/impl/permission.cpp
include/aegis/impl/snowflake.cpp
include/aegis/rest/impl/rest_controller.cpp
include/aegis/shards/impl/shard.cpp
include/aegis/shards/impl/shard_mgr.cpp
include/aegis/gateway/objects/impl/message.cpp)

if (AEGIS_DEBUG_HISTORY)
set(AEGIS_FLAGS ${AEGIS_FLAGS} AEGIS_DEBUG_HISTORY)
endif ()

if (AEGIS_PROFILING)
set(AEGIS_FLAGS ${AEGIS_FLAGS} AEGIS_PROFILING)
endif ()

if (AEGIS_DISABLE_ALL_CACHE)
set(AEGIS_FLAGS ${AEGIS_FLAGS} AEGIS_DISABLE_ALL_CACHE)
endif ()

if (AEGIS_HAS_STD_OPTIONAL)
set(AEGIS_FLAGS ${AEGIS_FLAGS} AEGIS_HAS_STD_OPTIONAL)
endif ()



set(AEGIS_PACKAGE_INCLUDE_DIRS
    $<BUILD_INTERFACE:${Asio_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${nlohmann_json_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${spdlog_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${Websocketpp_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${OpenSSL_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}>
)


include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Aegis)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


#SET(PKG_CONFIG_REQUIRES )
SET(PKG_CONFIG_LIBDIR
    "\${prefix}/lib"
)
SET(PKG_CONFIG_INCLUDEDIR
    "\${prefix}/include"
)

if (BUILD_SHARED_LIBS)

	add_library(aegis SHARED ${AEGIS_FILES})
	add_library(Aegis::aegis ALIAS aegis)

	set_target_properties(aegis PROPERTIES POSITION_INDEPENDENT_CODE 1)
	set_target_properties(aegis PROPERTIES SOVERSION 4)
	set_target_properties(aegis PROPERTIES LINKER_LANGUAGE CXX)

	target_compile_definitions(aegis PUBLIC AEGIS_DYN_LINK AEGIS_SOURCE ${AEGIS_FLAGS})
	target_compile_definitions(aegis PRIVATE AEGIS_SOURCE)

	target_link_libraries(aegis PUBLIC ${REQUIRED_LIBS})
	target_include_directories(aegis
	  PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		${AEGIS_PACKAGE_INCLUDE_DIRS}
	)

	if ((CXX_STANDARD EQUAL "17") OR (CMAKE_CXX_STANDARD EQUAL "17"))
		target_compile_features(aegis PUBLIC cxx_std_17)
		set_target_properties(aegis PROPERTIES CXX_STANDARD 17)
	else ()
		target_compile_features(aegis PUBLIC cxx_std_14)
		set_target_properties(aegis PROPERTIES CXX_STANDARD 14)
	endif ()

	set_target_properties(aegis PROPERTIES CXX_STANDARD_REQUIRED ON)

	target_compile_options(aegis PRIVATE ${AEGIS_CFLAGS})

	set_target_properties(aegis PROPERTIES EXPORT_NAME Aegis)

	install(TARGETS aegis
		EXPORT aegis-targets
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)

	SET(PKG_CONFIG_LIBS
		"-L\${libdir} -laegis -lssl -lcrypto -lpthread -lz -ldl"
	)
	SET(PKG_CONFIG_CFLAGS
		"-I/usr/include -I\${includedir} -I${Asio_INCLUDE_DIR} -I${JSON_INCLUDE_DIR} -I${spdlog_INCLUDE_DIR} -I${Websocketpp_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIR} -DAEGIS_DYN_LINK"
	)

	CONFIGURE_FILE(
	  "${CMAKE_SOURCE_DIR}/cmake/aegis.pc.cmake"
	  "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc"
	)

	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc" DESTINATION lib/pkgconfig)


else ()

	add_library(aegis_static STATIC ${AEGIS_FILES})
	add_library(Aegis::aegis ALIAS aegis_static)
	set_target_properties(aegis_static PROPERTIES OUTPUT_NAME "aegis")
	set_target_properties(aegis_static PROPERTIES LINKER_LANGUAGE CXX)
	target_compile_definitions(aegis_static PUBLIC AEGIS_SEPARATE_COMPILATION ${AEGIS_FLAGS})
	target_link_libraries(aegis_static INTERFACE ${REQUIRED_LIBS})
	target_include_directories(aegis_static
	  PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		${AEGIS_PACKAGE_INCLUDE_DIRS}
	)
	if ((CXX_STANDARD EQUAL "17") OR (CMAKE_CXX_STANDARD EQUAL "17"))
		target_compile_features(aegis_static PUBLIC cxx_std_17)
		set_target_properties(aegis_static PROPERTIES CXX_STANDARD 17)
	else ()
		target_compile_features(aegis_static PUBLIC cxx_std_14)
		set_target_properties(aegis_static PROPERTIES CXX_STANDARD 14)
	endif ()
	set_target_properties(aegis_static PROPERTIES CXX_STANDARD_REQUIRED ON)
	target_compile_options(aegis_static PRIVATE ${AEGIS_CFLAGS})

	install(TARGETS aegis_static
		EXPORT aegis-targets
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)

	SET(PKG_CONFIG_STATIC_LIBS
		"-L\${libdir} -l:libaegis.a -lssl -lcrypto -lpthread -lz -ldl"
	)
	SET(PKG_CONFIG_STATIC_CFLAGS
		"-I/usr/include -I\${includedir} -I${Asio_INCLUDE_DIR} -I${JSON_INCLUDE_DIR} -I${spdlog_INCLUDE_DIR} -I${Websocketpp_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIR} -DAEGIS_SEPARATE_COMPILATION"
	)

	CONFIGURE_FILE(
	  "${CMAKE_SOURCE_DIR}/cmake/aegis_static.pc.cmake"
	  "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc"
	)

	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc" DESTINATION lib/pkgconfig)

endif ()





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

if (BUILD_SHARED_LIBS)

	configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/AegisConfig.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/AegisConfig.cmake
		INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
		PATH_VARS AEGIS_PACKAGE_INCLUDE_DIRS
	)

else ()

	configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/AegisConfig_static.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/AegisConfig.cmake
		INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
		PATH_VARS AEGIS_PACKAGE_INCLUDE_DIRS
	)

endif ()

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AegisConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AegisConfigVersion.cmake
    ${CMAKE_CURRENT_LIST_DIR}/cmake/FindAsio.cmake
    ${CMAKE_CURRENT_LIST_DIR}/cmake/FindJSON.cmake
    ${CMAKE_CURRENT_LIST_DIR}/cmake/FindSpdlog.cmake
    ${CMAKE_CURRENT_LIST_DIR}/cmake/FindWebsocketpp.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)





export(EXPORT aegis-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/AegisTargets.cmake NAMESPACE Aegis::)

export(PACKAGE Aegis)


if (BUILD_EXAMPLES)

	add_executable(aegis_example src/example_main.cpp src/example.cpp)
	add_executable(aegis_minimal src/minimal.cpp)
	add_executable(aegis_headeronly_no_cache src/minimal.cpp)
	target_compile_definitions(aegis_headeronly_no_cache PRIVATE AEGIS_DISABLE_ALL_CACHE)

	set_property(TARGET aegis_example PROPERTY CXX_STANDARD 14)
	set_property(TARGET aegis_example PROPERTY CXX_STANDARD_REQUIRED ON)
	set_property(TARGET aegis_minimal PROPERTY CXX_STANDARD 14)
	set_property(TARGET aegis_minimal PROPERTY CXX_STANDARD_REQUIRED ON)
	set_property(TARGET aegis_headeronly_no_cache PROPERTY CXX_STANDARD 14)
	set_property(TARGET aegis_headeronly_no_cache PROPERTY CXX_STANDARD_REQUIRED ON)

	if(MSVC)
		target_link_libraries(aegis_example PRIVATE Aegis::aegis ${REQUIRED_LIBS})
		target_link_libraries(aegis_minimal PRIVATE Aegis::aegis ${REQUIRED_LIBS})
		target_link_libraries(aegis_headeronly_no_cache PRIVATE Aegis::aegis ${REQUIRED_LIBS})
	else()
		target_link_libraries(aegis_example PRIVATE Aegis::aegis ${REQUIRED_LIBS})
		target_link_libraries(aegis_minimal PRIVATE Aegis::aegis ${REQUIRED_LIBS})
		target_link_libraries(aegis_headeronly_no_cache PRIVATE Aegis::aegis ${REQUIRED_LIBS})
	endif()

	target_compile_options(aegis_example PRIVATE ${AEGIS_CFLAGS})
	target_compile_options(aegis_minimal PRIVATE ${AEGIS_CFLAGS})
	target_compile_options(aegis_headeronly_no_cache PRIVATE ${AEGIS_CFLAGS})

	target_include_directories(aegis_example
	  PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	)
	target_include_directories(aegis_minimal
	  PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	)
	target_include_directories(aegis_headeronly_no_cache
	  PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	)

endif ()
