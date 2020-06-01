# - Try to find Cairo
# Once done, this will define
#
#  CAIRO_FOUND - system has Cairo
#  CAIRO_INCLUDE_DIRS - the Cairo include directories
#  CAIRO_LIBRARIES - link these to use Cairo
#
# Copyright (C) 2012 Raphael Kubo da Costa <rakuco@webkit.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_CAIRO cairo) # FIXME: After we require CMake 2.8.2 we can pass QUIET to this call.

FIND_PATH(CAIRO_INCLUDE_DIR
    NAMES cairo.h
    HINTS ${PC_CAIRO_INCLUDEDIR}
          ${PC_CAIRO_INCLUDE_DIRS}
    PATH_SUFFIXES cairo
)

include(CheckSymbolExists)
set(CMAKE_REQUIRED_INCLUDES ${CAIRO_INCLUDE_DIR})
check_symbol_exists(CAIRO_HAS_WIN32_SURFACE cairo.h CAIRO_WIN32_FOUND)
check_symbol_exists(CAIRO_HAS_PNG_FUNCTIONS cairo.h CAIRO_PNG_FOUND)
check_symbol_exists(CAIRO_HAS_FT_FONT cairo.h CAIRO_FT_FOUND)
check_symbol_exists(CAIRO_HAS_FC_FONT cairo.h CAIRO_FC_FOUND)
check_symbol_exists(CAIRO_HAS_PS_SURFACE cairo.h CAIRO_PS_FOUND)
check_symbol_exists(CAIRO_HAS_PDF_SURFACE cairo.h CAIRO_PDF_FOUND)
check_symbol_exists(CAIRO_HAS_SVG_SURFACE cairo.h CAIRO_SVG_FOUND)

FIND_LIBRARY(CAIRO_LIBRARY
    NAMES cairo
    HINTS ${PC_CAIRO_LIBDIR}
          ${PC_CAIRO_LIBRARY_DIRS}
)

FIND_LIBRARY(CAIRO_STATIC_LIBRARIES
    NAMES cairo-static
    HINTS ${PC_CAIRO_LIBDIR}
          ${PC_CAIRO_LIBRARY_DIRS}
)
if(NOT CAIRO_LIBRARY AND CAIRO_STATIC_LIBRARIES)
    set(CAIRO_LIBRARY ${CAIRO_STATIC_LIBRARIES})
    set(CAIRO_C_FLAGS "-DCAIRO_WIN32_STATIC_BUILD")
endif()

IF (CAIRO_INCLUDE_DIR)
    IF (EXISTS "${CAIRO_INCLUDE_DIR}/cairo-version.h")
        FILE(READ "${CAIRO_INCLUDE_DIR}/cairo-version.h" CAIRO_VERSION_CONTENT)

        STRING(REGEX MATCH "#define +CAIRO_VERSION_MAJOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        SET(CAIRO_VERSION_MAJOR "${CMAKE_MATCH_1}")

        STRING(REGEX MATCH "#define +CAIRO_VERSION_MINOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        SET(CAIRO_VERSION_MINOR "${CMAKE_MATCH_1}")

        STRING(REGEX MATCH "#define +CAIRO_VERSION_MICRO +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        SET(CAIRO_VERSION_MICRO "${CMAKE_MATCH_1}")

        SET(CAIRO_VERSION "${CAIRO_VERSION_MAJOR}.${CAIRO_VERSION_MINOR}.${CAIRO_VERSION_MICRO}")
    ENDIF ()
ENDIF ()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cairo DEFAULT_MSG CAIRO_INCLUDE_DIR CAIRO_LIBRARY)
set(CAIRO_INCLUDE_DIRS ${CAIRO_INCLUDE_DIR})
set(CAIRO_LIBRARIES ${CAIRO_LIBRARY})