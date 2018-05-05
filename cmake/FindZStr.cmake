find_package(PkgConfig)
pkg_check_modules(PC_ZStr QUIET ZStr)

find_path(ZStr_INCLUDE_DIR
    NAMES zstr.hpp
    PATHS ${PC_ZStr_INCLUDE_DIRS}
    PATH_SUFFIXES zstr
)

if (ZStr_INCLUDE_DIR STREQUAL "ZStr_INCLUDE_DIR-NOTFOUND")
  message(WARNING "Using git-module path for ZStr")
  set(ZStr_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/zstr/src)
endif ()

mark_as_advanced(ZStr_FOUND ZStr_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZStr
    REQUIRED_VARS ZStr_INCLUDE_DIR
)

if(ZStr_FOUND)
    get_filename_component(ZStr_INCLUDE_DIRS ${ZStr_INCLUDE_DIR} DIRECTORY)
endif()

if(ZStr_FOUND AND NOT TARGET ZStr::ZStr)
    add_library(ZStr::ZStr INTERFACE IMPORTED)
    set_target_properties(ZStr::ZStr PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZStr_INCLUDE_DIRS}"
    )
endif()
