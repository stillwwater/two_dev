# Copyright (c) 2020 stillwwater
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
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

project(examples)

if (MSVC)
    set(CMAKE_CXX_WARNING_LEVEL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /GR-")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-exceptions -fno-rtti")
endif()

set(CMAKE_CXX_STANDARD 11)

add_definitions(
    -DTWO_ASSERTIONS
    -DTWO_LOGLEVEL=3
    -DTWO_DEBUG
)

set(TWO_EXAMPLES_MODULES
    sokoban.cpp
)

add_executable(examples ${TWO_EXAMPLES_MODULES})

add_subdirectory(../ two)

include_directories(
    ../src
    ../external
)

target_link_libraries(examples two)
