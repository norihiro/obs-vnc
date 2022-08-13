cmake_minimum_required(VERSION 3.12)

project(obs-vnc VERSION 0.4.0)

set(PLUGIN_AUTHOR "Norihiro Kamae")
set(MACOS_BUNDLEID "net.nagater.obs-vnc")
set(MACOS_PACKAGE_UUID "936A7E9F-1F28-4DE5-8553-7F03A266345B")
set(MACOS_INSTALLER_UUID "F7A275F7-2A5F-4578-8017-D841EB2C3DB0")
set(LINUX_MAINTAINER_EMAIL "norihiro@nagater.net")

# TAKE NOTE: No need to edit things past this point

# In case you need C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libobs REQUIRED)
include(cmake/ObsPluginHelpers.cmake)

# Linux and macOS; use pkg-config to find libvncclient
if(UNIX)
	find_package(PkgConfig)
	pkg_check_modules(LIBVNCCLIENT REQUIRED libvncclient)
endif()

if(OS_MACOS OR OS_WINDOWS)
	link_directories(${LIBVNCCLIENT_STATIC_LIBRARY_DIRS})
endif()

configure_file(
    src/plugin-macros.h.in
    ../src/plugin-macros.generated.h
)
configure_file(
    installer/installer-Windows.iss.in
    ../installer/installer-Windows.generated.iss
)

configure_file(
    ci/ci_includes.sh.in
    ../ci/ci_includes.generated.sh
)
configure_file(
    ci/ci_includes.cmd.in
    ../ci/ci_includes.generated.cmd
)

set(PLUGIN_SOURCES
	src/obs-vnc-source-main.c
	src/obs-vnc-source-thread.c
)

add_library(${CMAKE_PROJECT_NAME} MODULE ${PLUGIN_SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME}
	OBS::libobs
)

if(OS_WINDOWS)
	# Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when building with VS2019)
	if (MSVC)
		add_definitions(/MP /d2FH4-)
	endif()

	target_link_libraries(${CMAKE_PROJECT_NAME} OBS::w32-pthreads)

	target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${LIBVNCCLIENT_INCLUDE_DIRS})
	target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBVNCCLIENT_STATIC_LIBRARIES})
endif()

if(OS_LINUX)
	target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -g -Wall -Wextra)
	target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBVNCCLIENT_LIBRARIES})
endif()

if(OS_MACOS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fvisibility=default")

	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")

	configure_file(installer/installer-macOS.pkgproj.in installer-macOS.generated.pkgproj)

	target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${LIBVNCCLIENT_INCLUDE_DIRS})
	target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBVNCCLIENT_STATIC_LIBRARIES})
endif()

setup_plugin_target(${CMAKE_PROJECT_NAME})
