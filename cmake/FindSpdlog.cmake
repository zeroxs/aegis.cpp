find_package(PkgConfig)
pkg_check_modules(PC_Spdlog QUIET Spdlog)

find_path(Spdlog_INCLUDE_DIR
    NAMES "spdlog/spdlog.h"
    PATHS ${PC_Spdlog_INCLUDE_DIRS}
)

if (Spdlog_INCLUDE_DIR STREQUAL "Spdlog_INCLUDE_DIR-NOTFOUND")
  message(WARNING "Using git-module path for Spdlog")
  set(Spdlog_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/spdlog/include)
endif ()


file(READ ${Spdlog_INCLUDE_DIR}/spdlog/version.h version_h)

if (NOT version_h MATCHES "#define SPDLOG_VER_MAJOR ([0-9]+)")
  message(FATAL_ERROR "Cannot get Spdlog version from version.h.")
endif ()

math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})

if (NOT version_h MATCHES "#define SPDLOG_VER_MINOR ([0-9]+)")
  message(FATAL_ERROR "Cannot get Spdlog version from version.h.")
endif ()

math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_1})

if (NOT version_h MATCHES "#define SPDLOG_VER_PATCH ([0-9]+)")
  message(FATAL_ERROR "Cannot get Spdlog version from version.h.")
endif ()

math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_1})

string(CONCAT PC_Spdlog_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
                 ${CPACK_PACKAGE_VERSION_PATCH})

set(Spdlog_VERSION ${PC_Spdlog_VERSION})

mark_as_advanced(Spdlog_FOUND Spdlog_INCLUDE_DIR Spdlog_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Spdlog
    REQUIRED_VARS Spdlog_INCLUDE_DIR
    VERSION_VAR Spdlog_VERSION
)

if(Spdlog_FOUND)
    get_filename_component(Spdlog_INCLUDE_DIRS ${Spdlog_INCLUDE_DIR} DIRECTORY)
endif()

if(Spdlog_FOUND AND NOT TARGET Spdlog::Spdlog)
    add_library(Spdlog::Spdlog INTERFACE IMPORTED)
    set_target_properties(Spdlog::Spdlog PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Spdlog_INCLUDE_DIRS}"
    )
endif()
