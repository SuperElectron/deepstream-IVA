cmake_minimum_required(VERSION 3.15.2)

set(MODULE_NAME "softwareLicense")
message(STATUS "*** building ${MODULE_NAME} module ***")

##################################################
# get this module's files
file(GLOB HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# include this module's directory so other modules can find it
target_include_directories(${PROJECT_NAME}_LIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# create the library
target_sources(${PROJECT_NAME}_LIB PUBLIC ${HEADERS} ${SRC} )

# set location to where library folder is located
set(LIBRARY_FOLDER "/usr/local/lib" )
if( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    file(COPY "${CMAKE_BUILD_DIRECTORY}/.cache/lib/x86_64/libLexActivator.so" DESTINATION ${LIBRARY_FOLDER})
else(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "aarch64" )
    file(COPY "${CMAKE_BUILD_DIRECTORY}/.cache/lib/aarch64/libLexActivator.so" DESTINATION ${LIBRARY_FOLDER})
endif()
#
## set location to where library folder is located
#if( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
#    set(LIBRARY_FOLDER "${CMAKE_BUILD_DIRECTORY}/.cache/lib/x86_64/libLexActivator.so")
#else(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "aarch64" )
#    set(LIBRARY_FOLDER "${CMAKE_BUILD_DIRECTORY}/.cache/lib/aarch64/libLexActivator.so")
#endif()
message(STATUS "LIBRARY_FOLDER for libLexActivator.so: ${LIBRARY_FOLDER}")

# Set linker flags for dynamic linking
set(LDFLAGS "-Wl,-rpath,${LIBRARY_FOLDER};-L ${LIBRARY_FOLDER};-lLexActivator")

# link libs
target_link_libraries(${PROJECT_NAME}_LIB PRIVATE ${LDFLAGS})

## deferring work to test subdirectory if required
#if (BUILD_TESTS)
#    message(STATUS [BUILDING_${MODULE_NAME}_test_SUBDIRECTORY])
#    enable_testing()
#    add_subdirectory(test)
#endif ()

message(STATUS "*** finished building ${MODULE_NAME} module ***")

