find_package(PkgConfig)
pkg_check_modules(PC_PANGOWIN32 pangowin32 QUIET)

find_path(PANGOWIN32_INCLUDE_DIR
    NAMES
        pangowin32.h
    HINTS
        ${PC_PANGOWIN32_INCLUDEDIR}
        ${PC_PANGOWIN32_INCLUDE_DIRS}
    PATH_SUFFIXES
        pango
        pango-1.0
)

find_library(PANGOWIN32_LIBRARY
    NAMES
        pangowin32 libpangowin32
    HINTS
        ${PC_PANGOWIN32_LIBDIR}
        ${PC_PANGOWIN32_LIBRARIES}
    PATH_SUFFIXES
        pango
)
list(APPEND PANGOWIN32_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PANGOWIN32 DEFAULT_MSG 
    PANGOWIN32_LIBRARY PANGOWIN32_INCLUDE_DIR
)

mark_as_advanced(PANGOWIN32_INCLUDE_DIR PANGOWIN32_LIBRARY)
set(PANGOWIN32_INCLUDE_DIRS ${PANGOWIN32_INCLUDE_DIR})
set(PANGOWIN32_LIBRARIES ${PANGOWIN32_LIBRARY})