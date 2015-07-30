cmake_minimum_required(VERSION 2.8)
set(PROJECT_NAME leary)
project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") # c++ std version
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic-errors -Wno-unused-parameter") # debug flags
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2") # optimisation flags

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

include_directories(
	${CMAKE_SOURCE_DIR}/ext
	${CMAKE_SOURCE_DIR}/ext/glm	
)

add_subdirectory(ext)
add_subdirectory(src)

find_package(GLEW REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLEW_LIBRARIES})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL2_LIBRARY})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
