# Copyright (c) 2020 stillwwater
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
	message(FATAL_ERROR "Prevented in-tree build.")
endif()

cmake_minimum_required(VERSION 3.1)

project(two)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 11)

option(TWO_PARANOIA "Enable extra assertion checks" ON)

if (MSVC)
    set(CMAKE_CXX_WARNING_LEVEL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /GR-")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-exceptions -fno-rtti")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "TWO_DEBUG ON")
    add_definitions(
        -DTWO_ASSERTIONS
        -DTWO_LOGLEVEL=3
        -DTWO_DEBUG
        -DTWO_DEBUG_ENTITY
    )
    if (TWO_PARANOIA)
        message(STATUS "TWO_PARANOIA ON")
        add_definitions(-DTWO_PARANOIA)
    endif()
else()
    add_definitions(
        -DTWO_LOGLEVEL=1
    )
endif()

#
# Modules
#

set(TWO_SRC_MODULES
    src/debug.h
    src/debug.cpp
    src/filesystem.h
    src/filesystem.cpp
    src/entity.h
    src/entity.cpp
    src/image.h
    src/image.cpp
    src/sprite.h
    src/sprite.cpp
    src/text.h
    src/text.cpp
    src/two.h
    src/two.cpp
    src/main.cpp
)

include_directories(
    src
    external
)

add_executable(two ${TWO_SRC_MODULES})

#
# Third party libraries
#

#
# SDL2
#

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(external/SDL2)

#
# physfs
#

set(TWO_SRC_3P_PHYSFS
    external/physfs/physfs_archiver_7z.c
    external/physfs/physfs_archiver_dir.c
    external/physfs/physfs_archiver_grp.c
    external/physfs/physfs_archiver_hog.c
    external/physfs/physfs_archiver_iso9660.c
    external/physfs/physfs_archiver_mvl.c
    external/physfs/physfs_archiver_qpak.c
    external/physfs/physfs_archiver_slb.c
    external/physfs/physfs_archiver_unpacked.c
    external/physfs/physfs_archiver_vdf.c
    external/physfs/physfs_archiver_wad.c
    external/physfs/physfs_archiver_zip.c
    external/physfs/physfs_byteorder.c
    external/physfs/physfs_casefolding.h
    external/physfs/physfs_internal.h
    external/physfs/physfs_lzmasdk.h
    external/physfs/physfs_miniz.h
    external/physfs/physfs_platform_haiku.cpp
    external/physfs/physfs_platform_os2.c
    external/physfs/physfs_platform_posix.c
    external/physfs/physfs_platform_qnx.c
    external/physfs/physfs_platform_unix.c
    external/physfs/physfs_platform_windows.c
    external/physfs/physfs_platform_winrt.cpp
    external/physfs/physfs_platforms.h
    external/physfs/physfs_unicode.c
    external/physfs/physfs.c
	external/physfs/physfs.h
)

if (APPLE)
    set(TWO_SRC_3P_PHYSFS ${TWO_SRC_3P_PHYSFS}
        external/physfs/physfs_platform_apple.m
    )
endif()

add_library(two_3p_physfs ${TWO_SRC_3P_PHYSFS})

#
# stb
#

set(TWO_SRC_3P_STB
    external/stb/stb_image.h
)

set(TWO_3P
    SDL2main
    SDL2-static
    two_3p_physfs
)

target_link_libraries(two ${TWO_3P})
