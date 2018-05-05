find_package(PkgConfig)
pkg_check_modules(PC_Websocketpp QUIET Websocketpp)

find_path(Websocketpp_INCLUDE_DIR
    NAMES endpoint_base.hpp
    PATHS ${PC_Websocketpp_INCLUDE_DIRS}
    PATH_SUFFIXES websocketpp
)

if (Websocketpp_INCLUDE_DIR STREQUAL "Websocketpp_INCLUDE_DIR-NOTFOUND")
  message(WARNING "Using git-module path for Websocketpp")
  set(Websocketpp_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/websocketpp)
endif ()

file(READ ${Websocketpp_INCLUDE_DIR}/websocketpp/version.hpp version_hpp)

if (NOT version_hpp MATCHES "FOR_AEGIS")
  message(FATAL_ERROR "Using official Websocket++ source. Not compatible with libaegis due to missing features and Asio deprecation")
endif ()

if (NOT version_hpp MATCHES "static int const major_version = ([0-9]+);")
  message(FATAL_ERROR "Cannot get Websocketpp version from version.hpp.")
endif ()

math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})

if (NOT version_hpp MATCHES "static int const minor_version = ([0-9]+);")
  message(FATAL_ERROR "Cannot get Websocketpp version from version.hpp.")
endif ()

math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_1})

if (NOT version_hpp MATCHES "static int const patch_version = ([0-9]+);")
  message(FATAL_ERROR "Cannot get Websocketpp version from version.hpp.")
endif ()

math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_1})

string(CONCAT PC_Websocketpp_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
                 ${CPACK_PACKAGE_VERSION_PATCH})

set(Websocketpp_VERSION ${PC_Websocketpp_VERSION})

mark_as_advanced(Websocketpp_FOUND Websocketpp_INCLUDE_DIR Websocketpp_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Websocketpp
    REQUIRED_VARS Websocketpp_INCLUDE_DIR
    VERSION_VAR Websocketpp_VERSION
)

if(Websocketpp_FOUND)
    get_filename_component(Websocketpp_INCLUDE_DIRS ${Websocketpp_INCLUDE_DIR} DIRECTORY)
endif()

if(Websocketpp_FOUND AND NOT TARGET Websocketpp::Websocketpp)
    add_library(Websocketpp::Websocketpp INTERFACE IMPORTED)
    set_target_properties(Websocketpp::Websocketpp PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Websocketpp_INCLUDE_DIRS}"
    )
endif()
