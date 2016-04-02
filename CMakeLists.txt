################################################################################
# 
#  ooooo       ooooooooooo      o      oooooooooo ooooo  oooo 
#   888         888    88      888      888    888  888  88   
#   888         888ooo8       8  88     888oooo88     888     
#   888      o  888    oo    8oooo88    888  88o      888     
#  o888ooooo88 o888ooo8888 o88o  o888o o888o  88o8   o888o  
#
#-------------------------------------------------------------------------------
#
# @author: Jesper Stefansson (grouse)
# @email:  jesper.stefansson@gmail.com
#
# Copyright (c) 2015-2016 Jesper Stefansson
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
#    in a product, an acknowledgement in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
################################################################################

cmake_minimum_required(VERSION 3.0)

# Prevent in-tree builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-tree builds are prevented. Create a seperate build folder and run CMake from there.")
endif()



# ==============================================================================
# - Project setup
# ==============================================================================
project(leary)

LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (WIN32)
	set(ENV{SDL2DIR}       "${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2-2.0.4/")
	set(ENV{GLEW_LOCATION} "${CMAKE_CURRENT_SOURCE_DIR}/ext/GLEW-1.13.0/")
endif()



# ==============================================================================
# - Project source files
# ==============================================================================
list(APPEND leary_ENGINE_CORE_SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/engine/core/settings.cpp
)

list(APPEND leary_ENGINE_RENDER_SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/engine/render/rendering.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/engine/render/texture.cpp
)

list(APPEND leary_ENGINE_UTIL_SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/engine/util/debug.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/engine/util/environment.cpp
)

list(APPEND leary_ENGINE_SOURCE
	${leary_ENGINE_CORE_SOURCE}
	${leary_ENGINE_RENDER_SOURCE}
	${leary_ENGINE_UTIL_SOURCE}
)

list(APPEND leary_GAME_SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/engine/main.cpp
)



# ==============================================================================
# - Project header files
# ==============================================================================
list(APPEND leary_ENGINE_CORE_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/engine/core/settings.h
)

list(APPEND leary_ENGINE_RENDER_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/engine/render/rendering.h
	${CMAKE_CURRENT_SOURCE_DIR}/engine/render/texture.h
)

list(APPEND leary_ENGINE_UTIL_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/engine/util/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/engine/util/macros.h
	${CMAKE_CURRENT_SOURCE_DIR}/engine/util/environment.h
)

list(APPEND leary_ENGINE_HEADER
	${leary_ENGINE_CORE_HEADER}
	${leary_ENGINE_RENDER_HEADER}
	${leary_ENGINE_UTIL_HEADER}
)

list(APPEND leary_GAME_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/engine/prefix.h
)



# ==============================================================================
# - Project source groups
# ==============================================================================
source_group(Engine\\Core FILES 
	${leary_ENGINE_CORE_SOURCE}
	${leary_ENGINE_CORE_HEADER}
)

source_group(Engine\\Render FILES 
	${leary_ENGINE_RENDER_SOURCE}
	${leary_ENGINE_RENDER_HEADER}
)

source_group(Engine\\Util FILES 
	${leary_ENGINE_UTIL_SOURCE}
	${leary_ENGINE_UTIL_HEADER}
)

source_group(Game FILES 
	${leary_GAME_SOURCE}
	${leary_GAME_HEADER}
)



# ==============================================================================
# - Build executable
# ==============================================================================
add_executable(leary 
	${leary_ENGINE_SOURCE} 
	${leary_GAME_SOURCE}
	
	${leary_ENGINE_HEADER} 
	${leary_GAME_HEADER}
)



# ==============================================================================
# - Project definitions
# ==============================================================================
list(APPEND leary_DEFINITIONS_COMMON 
	GLEW_STATIC=1
)

list(APPEND leary_DEFINITIONS_DEBUG
	LEARY_DEBUG=1
	LEARY_RELEASE=0
)

list(APPEND leary_DEFINITIONS_RELEASE
	LEARY_DEBUG=0
	LEARY_RELEASE=1
)

# Platform definitions
if (WIN32)
	message(STATUS "Added definitions for platform: Windows")
	list(APPEND leary_DEFINITIONS_COMMON 
		LEARY_WIN=1
		LEARY_LINUX=0
	)
elseif (${CMAKE_SYSTEM} MATCHES Linux)
	message(STATUS "Added definitions for platform: Linux")
	list(APPEND leary_DEFINITIONS_COMMON
		LEARY_WIN=0
		LEARY_LINUX=1
	)
else ()
	message(WARNING "Unsupported platform detected")
	list(APPEND leary_DEFINITIONS_COMMON
		LEARY_WIN=0
		LEARY_LINUX=1
	)
endif ()


# Compiler definitions
if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
	message(STATUS "Added definitions for compiler: Clang") 
	list(APPEND leary_DEFINITIONS_COMMON
		DLEARY_COMPILER_MSVC=0
		LEARY_COMPILER_CLANG=1
		LEARY_COMPILER_GCC=0
	)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
	message(STATUS "Added definitions for compiler: GCC")
	list(APPEND leary_DEFINITIONS_COMMON
		LEARY_COMPILER_MSVC=0
		LEARY_COMPILER_CLANG=0
		LEARY_COMPILER_GCC=1
	)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	message(STATUS "Added definitions for compiler: MSVC")
	list(APPEND leary_DEFINITIONS_COMMON
		LEARY_COMPILER_MSVC=1
		LEARY_COMPILER_CLANG=0
		LEARY_COMPILER_GCC=0
	)
else ()
	message(WARNING "Unsupported compiler detected")
	list(APPEND leary_DEFINITIONS_COMMON
		LEARY_COMPILER_MSVC=0
		LEARY_COMPILER_CLANG=0
		LEARY_COMPILER_GCC=0
	)
endif ()
	
target_compile_definitions(leary
	PUBLIC ${leary_DEFINITIONS_COMMON}
	PUBLIC "$<$<CONFIG:Debug>:${leary_DEFINITIONS_DEBUG}>"
	PUBLIC "$<$<CONFIG:Release>:${leary_DEFINITIONS_RELEASE}>"
)




# ==============================================================================
# - Project compile options
# ==============================================================================
if (MSVC)
	list(APPEND PROJECT_COMPILE_OPTIONS
		-Wall

		/wd4820 # Implicit struct padding 
		/wd4514 # Unreferenced inline function has been removed
		/wd4710 # Function not inlined
		/wd4061 # Enum value not explicitly handled by case label
		/wd4996 # Unsafe function/variable (vsprintf vs vsprintf_s etc)
	)
else()
	list(APPEND PROJECT_COMPILE_OPTIONS
		-std=c++11
		-Weverything

	    -Wno-c++98-compat
	    -Wno-c++98-compat-pedantic
	    -Wno-unreachable-code
	    -Wno-old-style-cast
	    -Wno-padded
		-Wno-switch-enum
	)
endif()

target_compile_options(leary
	PRIVATE ${PROJECT_COMPILE_OPTIONS}
)



# ==============================================================================
# - External dependencies
# ==============================================================================
find_package(SDL2 REQUIRED)
if (SDL2_FOUND)
	list(APPEND leary_LIBRARIES ${SDL2_LIBRARY})
	list(APPEND leary_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
endif()

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
	list(APPEND leary_LIBRARIES ${OPENGL_LIBRARIES})
	list(APPEND leary_INCLUDE_DIRS ${OPENGL_INCLUDE_DIRS})
endif()

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
	list(APPEND leary_LIBRARIES ${GLEW_LIBRARY})
	list(APPEND leary_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
endif()

if (WIN32)
	find_library(SHLWAPI Shlwapi.lib)
	list(APPEND leary_LIBRARIES SHLWAPI)
endif()



# ==============================================================================
# - Include directories
# ==============================================================================
include_directories(
	engine
	ext/glm
	ext/SOIL/include

	SYSTEM
		${leary_INCLUDE_DIRS}
)



# ==============================================================================
# - Subprojects
# ==============================================================================
add_subdirectory(ext/SOIL)
list(APPEND leary_LIBRARIES SOIL)



# ==============================================================================
# - Link libraries
# ==============================================================================
string(REPLACE ";" "\n---- " leary_LIBRARIES_STR "\n---- ${leary_LIBRARIES}")
message("-- Libraries added to link step: ${leary_LIBRARIES_STR}")
target_link_libraries(leary ${leary_LIBRARIES})



# ==============================================================================
# - Post-build step
# ==============================================================================
if (MSVC)
	list(APPEND DLLs
		${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2-2.0.4/lib/x86_64/win/SDL2.dll
	)

	foreach(file ${DLLs})
		add_custom_command(TARGET leary POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy ${file} $<TARGET_FILE_DIR:leary>
		)
	endforeach()
endif()

add_custom_command(TARGET leary POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory 
		${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:leary>/data
)
