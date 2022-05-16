find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_HPSDR gnuradio-hpsdr)

FIND_PATH(
    GR_HPSDR_INCLUDE_DIRS
    NAMES gnuradio/hpsdr/api.h
    HINTS $ENV{HPSDR_DIR}/include
        ${PC_HPSDR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_HPSDR_LIBRARIES
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

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-hpsdrTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_HPSDR DEFAULT_MSG GR_HPSDR_LIBRARIES GR_HPSDR_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_HPSDR_LIBRARIES GR_HPSDR_INCLUDE_DIRS)
