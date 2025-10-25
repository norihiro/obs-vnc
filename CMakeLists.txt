cmake_minimum_required(VERSION 3.12)

project(obs-vnc VERSION 0.6.2)

set(PLUGIN_AUTHOR "Norihiro Kamae")
set(MACOS_BUNDLEID "net.nagater.obs-vnc")
set(MACOS_PACKAGE_UUID "936A7E9F-1F28-4DE5-8553-7F03A266345B")
set(MACOS_INSTALLER_UUID "F7A275F7-2A5F-4578-8017-D841EB2C3DB0")
set(PLUGIN_URL "https://obsproject.com/forum/resources/vnc-source.1000/")
set(LINUX_MAINTAINER_EMAIL "norihiro@nagater.net")

# TAKE NOTE: No need to edit things past this point

# In case you need C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	find_package(libobs REQUIRED)
	include(cmake/ObsPluginHelpers.cmake)
endif()

# Linux and macOS; use pkg-config to find libvncclient
option(USE_PKG_CONFIG "Use pkg-config to find libvncclient" ON)

if(USE_PKG_CONFIG)
	find_package(PkgConfig)
	if(PKG_CONFIG_FOUND)
		pkg_search_module(LIBVNCCLIENT libvncclient)
	endif()
endif()

if(LIBVNCCLIENT_FOUND)
	link_directories(${LIBVNCCLIENT_LIBRARY_DIRS})
else()
	link_directories(${LIBVNCCLIENT_STATIC_LIBRARY_DIRS})
endif()

configure_file(
	src/plugin-macros.h.in
	plugin-macros.generated.h
)

set(PLUGIN_SOURCES
	src/vnc-source-main.c
	src/vnc-source-thread.c
)

add_library(${PROJECT_NAME} MODULE ${PLUGIN_SOURCES})

target_include_directories(${PROJECT_NAME}
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME}
	OBS::libobs
)

if(LIBVNCCLIENT_FOUND)
	target_include_directories(${PROJECT_NAME} PRIVATE ${LIBVNCCLIENT_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${LIBVNCCLIENT_LIBRARIES})
else()
	target_include_directories(${PROJECT_NAME} PRIVATE ${LIBVNCCLIENT_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${LIBVNCCLIENT_STATIC_LIBRARIES})
endif()

if(OS_WINDOWS)
	# Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when building with VS2019)
	if (MSVC)
		add_definitions(/MP /d2FH4-)
		add_definitions("-D_CRT_SECURE_NO_WARNINGS") # for strncpy
		add_definitions("-D_USE_MATH_DEFINES") # for M_PI
	endif()

	target_link_libraries(${PROJECT_NAME} OBS::w32-pthreads)

	# Workaround to link OpenSSL library required by vncclient
	set(OPENSSL_USE_STATIC_LIBS ON)
	find_package(OpenSSL)
	if(OPENSSL_FOUND)
		# `crypt32` is required according to a description [1].
		# - [1] https://github.com/microsoft/vcpkg/pull/6309
		target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES} crypt32)
	endif()
endif()

if(OS_LINUX)
	target_compile_options(${PROJECT_NAME} PRIVATE -g -Wall -Wextra)
	target_link_options(${PROJECT_NAME} PRIVATE -Wl,--no-undefined)
endif()

if(OS_MACOS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fvisibility=default")

	set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

	set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
	set(MACOSX_PLUGIN_BUNDLE_VERSION "${PROJECT_VERSION}")
	set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")
endif()

setup_plugin_target(${PROJECT_NAME})

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	configure_file(
		installer/installer-Windows.iss.in
		installer-Windows.generated.iss
	)

	configure_file(
		ci/ci_includes.sh.in
		ci/ci_includes.generated.sh
	)

	configure_file(
		installer/installer-macOS.pkgproj.in
		installer-macOS.generated.pkgproj
	)
endif()
