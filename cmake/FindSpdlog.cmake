find_package(PkgConfig)
pkg_check_modules(PC_Spdlog QUIET Spdlog)

find_path(Spdlog_INCLUDE_DIR
    NAMES spdlog.h
    PATHS ${PC_Spdlog_INCLUDE_DIRS}
    PATH_SUFFIXES spdlog
)

if (Spdlog_INCLUDE_DIR STREQUAL "Spdlog_INCLUDE_DIR-NOTFOUND")
  message(WARNING "Using git-module path for Spdlog")
  set(Spdlog_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/spdlog/include)
else ()
  get_filename_component(Spdlog_INCLUDE_DIR ${Spdlog_INCLUDE_DIR} DIRECTORY)
endif ()

file(READ ${Spdlog_INCLUDE_DIR}/spdlog/common.h common_h)
if (NOT common_h MATCHES "SPDLOG_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)-?(.*)\"")
  message(FATAL_ERROR "Cannot get SPDLOG_VERSION from common.h.")
endif ()
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
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
