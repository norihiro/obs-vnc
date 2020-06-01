find_package(PkgConfig)
pkg_check_modules(PC_PANGO pango QUIET)
if(PC_PANGO_FOUND)
	find_package_handle_standard_args(PANGO DEFAULT_MSG
		PC_PANGO_INCLUDE_DIRS PC_PANGO_LIBRARIES
	)

	mark_as_advanced(PC_PANGO_FOUND)
	set(PANGO_INCLUDE_DIRS ${PC_PANGO_INCLUDE_DIRS})
	set(PANGO_LIBRARIES ${PC_PANGO_LIBRARIES})
	set(PANGO_LIBRARY_DIRS ${PC_PANGO_LIBRARY_DIRS})
	return()
endif()

# Try manually if no pkg-config
find_path(PANGO_INCLUDE_DIR
    NAMES
        pango/pango.h
    HINTS
        ${PC_PANGO_INCLUDEDIR}
        ${PC_PANGO_INCLUDE_DIRS}
)

find_library(PANGO_LIBRARY
    NAMES
        pango libpango pango-1.0
    HINTS
        ${PC_PANGO_LIBDIR}
        ${PC_PANGO_LIBRARIES}
    PATH_SUFFIXES
        pango
)

pkg_check_modules(PC_GLIB REQUIRED glib-2.0)
pkg_check_modules(PC_GOBJECT REQUIRED gobject-2.0)
pkg_check_modules(PC_GMODULE REQUIRED gmodule-2.0)

if(PANGO_LIBRARY) # avoid false positive finds
    list(APPEND PANGO_LIBRARY ${PC_GLIB_LIBRARIES} ${PC_GOBJECT_LIBRARIES} ${PC_GMODULE_LIBRARIES})
    list(APPEND PANGO_INCLUDE_DIR ${PC_GLIB_INCLUDE_DIRS} ${PC_OBJECT_INCLUDE_DIRS} ${PC_GMODULE_INCLUDE_DIRS})
    list(APPEND PANGO_LIBRARY_DIR ${PC_GLIB_LIBRARY_DIRS} ${PC_GOBJECT_LIBRARY_DIRS} ${PC_GMODULE_LIBRARY_DIRS})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PANGO DEFAULT_MSG 
    PANGO_INCLUDE_DIR PANGO_LIBRARY
)

mark_as_advanced(PANGO_INCLUDE_DIR PANGO_LIBRARY PANGO_LIBRARY_DIR)
set(PANGO_INCLUDE_DIRS ${PANGO_INCLUDE_DIR})
set(PANGO_LIBRARIES ${PANGO_LIBRARY})
set(PANGO_LIBRARY_DIRS ${PANGO_LIBRARY_DIR})
