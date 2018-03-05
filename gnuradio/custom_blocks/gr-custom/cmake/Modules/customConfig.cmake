INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_CUSTOM custom)

FIND_PATH(
    CUSTOM_INCLUDE_DIRS
    NAMES custom/api.h
    HINTS $ENV{CUSTOM_DIR}/include
        ${PC_CUSTOM_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    CUSTOM_LIBRARIES
    NAMES gnuradio-custom
    HINTS $ENV{CUSTOM_DIR}/lib
        ${PC_CUSTOM_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CUSTOM DEFAULT_MSG CUSTOM_LIBRARIES CUSTOM_INCLUDE_DIRS)
MARK_AS_ADVANCED(CUSTOM_LIBRARIES CUSTOM_INCLUDE_DIRS)

