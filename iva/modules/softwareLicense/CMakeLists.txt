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
if( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    set(LIBRARY_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/libraries/x86_64/libLexActivator.so" )
else(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "aarch64" )
    set(LIBRARY_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/libraries/aarch64/libLexActivator.so" )
endif()

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

