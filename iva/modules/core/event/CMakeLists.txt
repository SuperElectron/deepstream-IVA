cmake_minimum_required(VERSION 3.15.2)

set(MODULE_NAME "CORE.event")
message(STATUS "*** building ${MODULE_NAME} module ***")

# get the source files
file(GLOB HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# include this module's directory so other modules can find it
target_include_directories(${PROJECT_NAME}_LIB PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

# create the library
target_sources(${PROJECT_NAME}_LIB PUBLIC
        ${HEADERS}
        ${SRC}
        )

# nlohamnn's json
if (INCLUDE_NLOHMANN_JSON)
    include(${NLOHMANN_JSON_CMAKE_DIR})
    set(INCLUDE_NLOHMANN_JSON FALSE PARENT_SCOPE)
endif ()

if (INCLUDE_UUID)
    include(${UUID_CMAKE_DIR})
    set(INCLUDE_GSTREAMER FALSE PARENT_SCOPE)
endif ()

## deferring work to test subdirectory if required
#if (BUILD_TESTS)
#    message(STATUS [BUILDING_${MODULE_NAME}_test_SUBDIRECTORY])
#    enable_testing()
#    add_subdirectory(test)
#endif ()

message(STATUS "*** finished ${MODULE_NAME} module ***")