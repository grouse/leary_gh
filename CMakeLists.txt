#===================================================================================================
#
#  ooooo       ooooooooooo      o      oooooooooo ooooo  oooo 
#   888         888    88      888      888    888  888  88   
#   888         888ooo8       8  88     888oooo88     888     
#   888      o  888    oo    8oooo88    888  88o      888     
#  o888ooooo88 o888ooo8888 o88o  o888o o888o  88o8   o888o  
#
#---------------------------------------------------------------------------------------------------
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
####################################################################################################
#---------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.0)

# Prevent in-tree builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-tree builds are prevented. Create a seperate build folder and run CMake from there.")
endif()



#===================================================================================================
# - Project setup
#---------------------------------------------------------------------------------------------------
project(leary)

LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(LEARY_VULKAN     "Vulkan renderer"           ON)
option(LEARY_OPENGL     "OpenGL renderer"           OFF)
option(LEARY_LOG_ENABLE "Enable game debug logging" ON)


#===================================================================================================
# - Project source files
#---------------------------------------------------------------------------------------------------
list(APPEND leary_ENGINE_CORE_SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/src/core/settings.cpp
)

list(APPEND leary_ENGINE_RENDER_SOURCE
)

if (LEARY_OPENGL)
	list(APPEND leary_ENGINE_RENDER_SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/opengl/opengl_device.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/opengl/opengl_shader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/texture.cpp
	)
endif()

if (LEARY_VULKAN)
	list(APPEND leary_ENGINE_RENDER_SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/vulkan/vulkan_device.cpp
	)
endif()

if (WIN32)
	list(APPEND leary_ENGINE_PLATFORM_SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win32_main.cpp
	)
elseif (${CMAKE_SYSTEM} MATCHES Linux)
	list(APPEND leary_ENGINE_PLATFORM_SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux_main.cpp
	)
endif()

list(APPEND leary_ENGINE_SOURCE
	${leary_ENGINE_CORE_SOURCE}
	${leary_ENGINE_RENDER_SOURCE}
	${leary_ENGINE_PLATFORM_SOURCE}
)

list(APPEND leary_GAME_SOURCE
)



#===================================================================================================
# - Project header files
#---------------------------------------------------------------------------------------------------
list(APPEND leary_ENGINE_CORE_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/src/core/settings.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/core/ecs.h
)

list(APPEND leary_ENGINE_RENDER_HEADER
)

if (LEARY_OPENGL)
	list(APPEND leary_ENGINE_RENDER_HEADER
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/opengl/opengl_device.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/opengl/opengl_shader.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/texture.h
	)
endif()

if (LEARY_VULKAN)
	list(APPEND leary_ENGINE_RENDER_HEADER
		${CMAKE_CURRENT_SOURCE_DIR}/src/render/vulkan/vulkan_device.h
	)
endif()

list(APPEND leary_ENGINE_PLATFORM_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/src/platform/file.h
)

list(APPEND leary_ENGINE_HEADER
	${leary_ENGINE_CORE_HEADER}
	${leary_ENGINE_RENDER_HEADER}
	${leary_ENGINE_PLATFORM_HEADER}
)

list(APPEND leary_GAME_HEADER
)



#==================================================================================================
# - Project source groups
#---------------------------------------------------------------------------------------------------
source_group(src\\core FILES
	${leary_ENGINE_CORE_SOURCE}
	${leary_ENGINE_CORE_HEADER}
)

source_group(src\\render FILES 
	${leary_ENGINE_RENDER_SOURCE}
	${leary_ENGINE_RENDER_HEADER}
)

source_group(game FILES 
	${leary_GAME_SOURCE}
	${leary_GAME_HEADER}
)



#==================================================================================================
# - Build executable
#---------------------------------------------------------------------------------------------------
add_executable(leary
	${leary_ENGINE_SOURCE} 
	${leary_GAME_SOURCE}
	
	${leary_ENGINE_HEADER} 
	${leary_GAME_HEADER}
)



#==================================================================================================
# - Project definitions
#---------------------------------------------------------------------------------------------------
list(APPEND leary_DEFINITIONS_COMMON
	ON=1  # define ON/OFF as integers so we can pass cmake options directly to project defines and
	OFF=0 # use them in #if preprocessor statements

	LEARY_OPENGL=${LEARY_OPENGL}
	LEARY_VULKAN=${LEARY_VULKAN}
	LEARY_LOG_ENABLE=${LEARY_LOG_ENABLE}
)

list(APPEND leary_DEFINITIONS_DEBUG
	LEARY_DEBUG=1
	LEARY_RELEASE=0
)

list(APPEND leary_DEFINITIONS_RELEASE
	LEARY_DEBUG=0
	LEARY_RELEASE=1
)

if (MSVC)
	list(APPEND leary_DEFINITIONS_COMMON
		NOMINMAX
	)
endif()
	

target_compile_definitions(leary
	PUBLIC ${leary_DEFINITIONS_COMMON}
	PUBLIC "$<$<CONFIG:Debug>:${leary_DEFINITIONS_DEBUG}>"
	PUBLIC "$<$<CONFIG:Release>:${leary_DEFINITIONS_RELEASE}>"
)



#==================================================================================================
# - Project compile options
#---------------------------------------------------------------------------------------------------
if (MSVC)
	list(APPEND PROJECT_COMPILE_OPTIONS
		-Wall

		/wd4820 # Implicit struct padding 
		/wd4514 # Unreferenced inline function has been removed
		/wd4710 # Function not inlined
		/wd4061 # Enum value not explicitly handled by case label
		/wd4996 # Unsafe function/variable (vsprintf vs vsprintf_s etc)
	)

	set_target_properties(leary PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
else()
	list(APPEND PROJECT_COMPILE_OPTIONS
		-std=c++11
		-Wall
		-Wextra

	    -Wno-c++98-compat
	    -Wno-c++98-compat-pedantic
	    -Wno-unreachable-code
	    -Wno-old-style-cast
	    -Wno-padded
		-Wno-switch-enum
		-Wno-documentation
	)
endif()

add_compile_options(-w)
target_compile_options(leary
	PRIVATE ${PROJECT_COMPILE_OPTIONS}
)



#==================================================================================================
# - External dependencies
#---------------------------------------------------------------------------------------------------
if (LEARY_OPENGL)
	find_package(OpenGL REQUIRED)
	if (OPENGL_FOUND)
		list(APPEND leary_LIBRARIES ${OPENGL_LIBRARIES})
		list(APPEND leary_INCLUDE_DIRS ${OPENGL_INCLUDE_DIRS})
	endif()
endif()

if (LEARY_VULKAN)
	find_package(Vulkan REQUIRED)
	if (VULKAN_FOUND)

		list(APPEND leary_LIBRARIES 
			${VULKAN_LIBRARY}
		)

		list(APPEND leary_INCLUDE_DIRS ${VULKAN_INCLUDE_DIR})

	endif()
endif()

if (WIN32)
	find_library(SHLWAPI Shlwapi.lib)
	list(APPEND leary_LIBRARIES SHLWAPI)
endif()

if (${CMAKE_SYSTEM} MATCHES Linux)
	find_package(XCB REQUIRED)
	if (XCB_FOUND)
		list(APPEND leary_LIBRARIES ${XCB_LIBRARY})
		list(APPEND leary_INCLUDE_DIRS ${XCB_INCLUDE_DIR})
	endif()
endif()


#==================================================================================================
# - Include directories
#---------------------------------------------------------------------------------------------------
include_directories(leary
	PRIVATE
	    src
		${leary_INCLUDE_DIRS}
)



#==================================================================================================
# - Link libraries
#---------------------------------------------------------------------------------------------------
string(REPLACE ";" "\n---- " leary_LIBRARIES_STR "\n---- ${leary_LIBRARIES}")
message("-- Libraries added to link step: ${leary_LIBRARIES_STR}")
target_link_libraries(leary ${leary_LIBRARIES})



#==================================================================================================
# - Post-build step
#---------------------------------------------------------------------------------------------------
add_custom_command(TARGET leary POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory 
		${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:leary>/data
)
