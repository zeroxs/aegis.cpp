find_package(PkgConfig)
pkg_check_modules(PC_JSON QUIET JSON)

find_path(JSON_INCLUDE_DIR
    NAMES "nlohmann/json.hpp"
    PATHS ${PC_JSON_INCLUDE_DIRS}
)

if (JSON_INCLUDE_DIR STREQUAL "JSON_INCLUDE_DIR-NOTFOUND")
  message(WARNING "Using git-module path for JSON")
  set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include)
  set(JSON_SUBDIR true)
endif ()

file(READ ${JSON_INCLUDE_DIR}/nlohmann/json.hpp json_hpp)
if (NOT json_hpp MATCHES "version ([0-9]+)\\.([0-9]+)\\.([0-9]+)")
  message(FATAL_ERROR "Cannot get JSON_VERSION from json.hpp.")
endif ()
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
string(CONCAT PC_JSON_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
                 ${CPACK_PACKAGE_VERSION_PATCH})

set(JSON_VERSION ${PC_JSON_VERSION})

mark_as_advanced(JSON_FOUND JSON_INCLUDE_DIR JSON_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JSON
    REQUIRED_VARS JSON_INCLUDE_DIR
    VERSION_VAR JSON_VERSION
)

if (JSON_SUBDIR)
  set(JSON_INCLUDE_DIRS ${JSON_INCLUDE_DIR})
else()
  if(JSON_FOUND)
    get_filename_component(JSON_INCLUDE_DIRS ${JSON_INCLUDE_DIR} DIRECTORY)
  endif()
endif()

if(JSON_FOUND AND NOT TARGET JSON::JSON)
    add_library(JSON::JSON INTERFACE IMPORTED)
    set_target_properties(JSON::JSON PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${JSON_INCLUDE_DIRS}"
    )
endif()
