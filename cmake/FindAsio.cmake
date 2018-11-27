find_package(PkgConfig)
pkg_check_modules(PC_Asio QUIET Asio)

find_path(Asio_INCLUDE_DIR
    NAMES asio.hpp
    PATHS ${PC_Asio_INCLUDE_DIRS}
)

if (Asio_INCLUDE_DIR STREQUAL "Asio_INCLUDE_DIR-NOTFOUND")
  message(FATAL_ERROR "Cannot find ASIO")
endif ()

file(READ ${Asio_INCLUDE_DIR}/asio/version.hpp version_hpp)
if (NOT version_hpp MATCHES "ASIO_VERSION ([0-9])([0-9][0-9][0-9])([0-9][0-9])")
  message(FATAL_ERROR "Cannot get ASIO_VERSION from version.hpp. ${Asio_INCLUDE_DIR}/asio/version.hpp")
endif ()
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
string(CONCAT PC_Asio_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
                 ${CPACK_PACKAGE_VERSION_PATCH})

set(Asio_VERSION ${PC_Asio_VERSION})

mark_as_advanced(Asio_FOUND Asio_INCLUDE_DIR Asio_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Asio
    REQUIRED_VARS Asio_INCLUDE_DIR
    VERSION_VAR Asio_VERSION
)

if(Asio_FOUND)
    get_filename_component(Asio_INCLUDE_DIRS ${Asio_INCLUDE_DIR} DIRECTORY)
endif()

if(Asio_FOUND AND NOT TARGET Asio::Asio)
    add_library(Asio::Asio INTERFACE IMPORTED)
    set_target_properties(Asio::Asio PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Asio_INCLUDE_DIRS}"
    )
endif()
