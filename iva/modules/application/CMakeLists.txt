cmake_minimum_required(VERSION 3.15.2)

set(MODULE_NAME "application")
message(STATUS "*** building ${MODULE_NAME} module ***")

# get the source files
file(GLOB HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# include this module's directory so other modules can find it
target_include_directories(${PROJECT_NAME}_LIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# create the library
target_sources(${PROJECT_NAME}_LIB PUBLIC
        ${HEADERS}
        ${SRC}
        )

message(STATUS "*** finished building ${MODULE_NAME} module ***")