prefix=${CMAKE_INSTALL_PREFIX}
includedir=${PKG_CONFIG_INCLUDEDIR}
libdir=${PKG_CONFIG_LIBDIR}

Name: Aegis Library
Description: Aegis C++ Discord Library
Version: ${PROJECT_VERSION}
Requires: ${PKG_CONFIG_REQUIRES}
Libs: ${PKG_CONFIG_STATIC_LIBS}
Cflags: ${PKG_CONFIG_STATIC_CFLAGS}
