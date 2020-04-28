INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_HPSDR hpsdr)

FIND_PATH(
    HPSDR_INCLUDE_DIRS
    NAMES hpsdr/api.h
    HINTS $ENV{HPSDR_DIR}/include
        ${PC_HPSDR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    HPSDR_LIBRARIES
    NAMES gnuradio-hpsdr
    HINTS $ENV{HPSDR_DIR}/lib
        ${PC_HPSDR_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/hpsdrTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HPSDR DEFAULT_MSG HPSDR_LIBRARIES HPSDR_INCLUDE_DIRS)
MARK_AS_ADVANCED(HPSDR_LIBRARIES HPSDR_INCLUDE_DIRS)
