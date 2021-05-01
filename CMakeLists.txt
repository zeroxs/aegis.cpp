message(STATUS "CMake version: ${CMAKE_VERSION}")

cmake_minimum_required(VERSION 3.14)

include(FetchContent)
include(ExternalProject)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/include/aegis/version.hpp version_hpp)
if (NOT version_hpp MATCHES "AEGIS_VERSION_LONG \"([0-9]+)\.([0-9]+)\.([0-9]+) ([0-9]+\/[0-9]+\/[0-9]+)\"")
  message(FATAL_ERROR "Cannot get AEGIS_VERSION_LONG from version.hpp. ${CMAKE_CURRENT_SOURCE_DIR}/include/aegis/version.hpp")
endif ()
math(EXPR AEGIS_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR AEGIS_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR AEGIS_VERSION_PATCH ${CMAKE_MATCH_3})
set(AEGIS_VERSION_BUILD ${CMAKE_MATCH_4})

string(CONCAT AEGIS_VERSION ${AEGIS_VERSION_MAJOR}.${AEGIS_VERSION_MINOR}.${AEGIS_VERSION_PATCH})

message(STATUS "aegis.cpp version: ${AEGIS_VERSION} release date ${AEGIS_VERSION_BUILD}")

set(USE_TLS TRUE)
set(USE_OPEN_SSL TRUE)

set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,--no-undefined")
set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(AEGIS_CFLAGS -Wno-sign-compare -Wno-unknown-pragmas -Wno-unused-variable -Wno-noexcept-type -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-missing-field-initializers)
add_compile_options("$<$<CONFIG:RELEASE>:-O2>")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_compile_options("-stdlib=libc++")
else()
	# nothing special for gcc at the moment
endif()

project(libaegis VERSION ${AEGIS_VERSION} LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenSSL 1.1.1 REQUIRED MODULE)
find_package(ZLIB REQUIRED MODULE)

ExternalProject_Add(ixws
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG  v11.2.4
)
ExternalProject_Get_Property(ixws SOURCE_DIR BINARY_DIR)

message("ixwebsocket source directory: ${SOURCE_DIR}")
message("ixwebsocket binary directory: ${BINARY_DIR}")
set(ixwebsocket_SOURCE_DIR ${SOURCE_DIR})
set(ixwebsocket_LIB_DIR ${BINARY_DIR})

#add_library(ixwebsocket SHARED IMPORTED)
#set_target_properties(ixwebsocket PROPERTIES IMPORTED_LOCATION ${ixwebsocket_LIB_DIR}/libixwebsocket.a)

find_package(ixwebsocket REQUIRED)

set(REQUIRED_LIBS ixwebsocket::ixwebsocket OpenSSL::SSL ZLIB::ZLIB dl Threads::Threads)

option(BUILD_SHARED_LIBS "Build the shared library" ON)
option(BUILD_EXAMPLES "Build example programs" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_BUILD_TYPE Release)
set(AEGIS_FLAGS )

set(AEGIS_FILES
src/aegis.cpp
src/core.cpp
include/aegis/vendor/simdjson/simdjson.cpp
)

set(AEGIS_PACKAGE_INCLUDE_DIRS
    $<BUILD_INTERFACE:${OpenSSL_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ixwebsocket_SOURCE_DIR}>
)

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Aegis)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

SET(PKG_CONFIG_INCLUDEDIR
    "\${prefix}/include"
)

if (BUILD_SHARED_LIBS)

	add_library(aegis SHARED ${AEGIS_FILES})
	add_library(Aegis::aegis ALIAS aegis)
	add_dependencies(aegis ixws)


	set_target_properties(aegis PROPERTIES POSITION_INDEPENDENT_CODE 1)
	set_target_properties(aegis PROPERTIES SOVERSION ${AEGIS_VERSION_MAJOR})
	set_target_properties(aegis PROPERTIES LINKER_LANGUAGE CXX)

	target_compile_definitions(aegis PUBLIC AEGIS_DYN_LINK AEGIS_SOURCE ${AEGIS_FLAGS})
	target_compile_definitions(aegis PRIVATE AEGIS_SOURCE)

	target_link_libraries(aegis PUBLIC ${REQUIRED_LIBS})
	target_include_directories(aegis
	  PUBLIC
		$<INSTALL_INTERFACE:include>    
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		${AEGIS_PACKAGE_INCLUDE_DIRS}
		${IXWEBSOCKET_INCLUDE_DIRS}
	)

	target_compile_features(aegis PUBLIC cxx_std_17)

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
		"-I/usr/include -I\${includedir}"
	)

	CONFIGURE_FILE(
	  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/aegis.pc.cmake"
	  "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc"
	)

	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/aegis.pc" DESTINATION lib/pkgconfig)

else ()

	add_library(aegis_static STATIC ${AEGIS_FILES})
	add_library(Aegis::aegis ALIAS aegis_static)
	add_dependencies(aegis ixws)

	set_target_properties(aegis_static PROPERTIES OUTPUT_NAME "aegis")
	set_target_properties(aegis_static PROPERTIES LINKER_LANGUAGE CXX)
	target_compile_definitions(aegis_static PUBLIC ${AEGIS_FLAGS})
	target_link_libraries(aegis_static PUBLIC ${REQUIRED_LIBS})
	target_include_directories(aegis_static
	  PUBLIC
		$<INSTALL_INTERFACE:include>    
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		${AEGIS_PACKAGE_INCLUDE_DIRS}
	)
	target_compile_features(aegis_static PUBLIC cxx_std_17)

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
		"-I/usr/include -I\${includedir}"
	)

	CONFIGURE_FILE(
	  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/aegis_static.pc.cmake"
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
    DESTINATION ${INSTALL_CONFIGDIR}
)

export(EXPORT aegis-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/AegisTargets.cmake NAMESPACE Aegis::)

export(PACKAGE Aegis)

if (BUILD_EXAMPLES)

	add_executable(aegis_example examples/example.cpp)

	set_target_properties(aegis_example PROPERTIES LINKER_LANGUAGE CXX)
	set_property(TARGET aegis_example PROPERTY CXX_STANDARD 17)
	set_property(TARGET aegis_example PROPERTY CXX_STANDARD_REQUIRED ON)
	target_link_libraries(aegis_example PRIVATE Aegis::aegis ${REQUIRED_LIBS})
	target_compile_options(aegis_example PRIVATE ${AEGIS_CFLAGS})
	target_include_directories(aegis_example
	  PUBLIC
		$<INSTALL_INTERFACE:include>    
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    	$<BUILD_INTERFACE:${ixwebsocket_SOURCE_DIR}>
	)
endif ()
