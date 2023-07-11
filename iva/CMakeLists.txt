################################################
### BUILD instructions (generate binary in /src/build)
## cmake [options]
# -DBUILD_TESTS=ON/OFF
# cmake -B [directory] -S [location of CMakeLists.txt] [options]
# cmake -B build -S . -DBUILD_TESTS=OFF
# cmake --build [directory] [-j(number of processes), -j=all]
# cmake --build build -j
### RUN instructions (run binary file `gstreamer` in /src/build)
# cd build
# ./gstreamer
################################################
# Main CMake settings for the project
################################################
cmake_minimum_required(VERSION 3.15.2)
enable_language(C CXX)


# Set build type (Release, Debug, RelWithDebInfo, MinSizeRel)
#  https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
set(CMAKE_BUILD_TYPE RelWithDebInfo)

# set PROJECT_LIB descriptors
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "AlphaWise - Deepstream IVA")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

################################################
# set directories
################################################

set(CMAKE_CONFIGURATION_TYPES UNIX)
set(CMAKE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/bin)
set(CMAKE_LIBRARY_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(PROJECT_MODULES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/modules)

################################################
# Set up packages and build options
################################################

set("CMAKE_TOOLCHAIN_FILE /start/vcpkg/scripts/buildsystems/vcpkg.cmake")
include(/start/vcpkg/scripts/buildsystems/vcpkg.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_MODULES_DIR}")
# cmake .. -DBUILD_TESTS=ON
option(BUILD_TESTS "Build unit tests" OFF)

################################################
# Configure project executable and library (static or shared)
################################################

project(iva DESCRIPTION ${PROJECT_DESCRIPTION} VERSION ${PROJECT_VERSION})
add_library(${PROJECT_NAME}_LIB STATIC)
# https://cliutils.gitlab.io/modern-cmake/chapters/features/small.html
set_target_properties(${PROJECT_NAME}_LIB PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME}_LIB PROPERTIES LINKER_LANGUAGE CXX)

set(SRC_FILE main.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILE})

################################################
# Test configuration
################################################

# bootstrap gtest
if(BUILD_TESTS)
    message(STATUS [BUILDING_TEST_SUBDIRECTORY])
    # configure build of googletest
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
    target_link_libraries(${PROJECT_NAME}_LIB PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
    # set GTest configs
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    find_program(GCOV_PATH gcov)
    if(NOT GCOV_PATH)
        message(FATAL_ERROR "Code coverage analysis requires gcov!")
    endif()
    enable_testing()
endif()

################################################
# Build sub-modules (/src/modules/<module-name>)
################################################

add_subdirectory(modules/core)
add_subdirectory(modules/application)
add_subdirectory(modules/gst)
add_subdirectory(modules/messaging)


################################################
# Include libraries base cmakes for all modules
################################################

include(cmake/date.cmake)
include(cmake/glog.cmake)
include(cmake/gstreamer.cmake)
include(cmake/nlohmannjson.cmake)
include(cmake/nvds.cmake)
include(cmake/opencv.cmake)
include(cmake/threadpool.cmake)
include(cmake/uuid.cmake)
include(cmake/yaml.cmake)

################################################
## Build Application
################################################

set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD 20)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_features(${PROJECT_NAME}_LIB PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_LIB ${CMAKE_DL_LIBS})

################################################
# Compile tests
################################################
if (BUILD_TESTS)
    message(STATUS BUILDING [application_test])
    add_subdirectory(modules/application/test)
    message(STATUS BUILDING [gst.pipeline_test])
    add_subdirectory(modules/gst/pipeline/test)
    message(STATUS BUILDING [gst.processing_test])
    add_subdirectory(modules/gst/processing/test)
    message(STATUS BUILDING [messaging_test])
    add_subdirectory(modules/messaging/test)
endif ()

################################################
# Set additional library properties
################################################

set_target_properties(${PROJECT_NAME}_LIB PROPERTIES
        VERSION ${PROJECT_VERSION}
        LINK_FLAGS "-Wl,--whole-archive"
)
target_include_directories(${PROJECT_NAME}_LIB PUBLIC ${PROJECT_MODULES_DIRECTORY})
include(GNUInstallDirs)
message(CMAKE_INSTALL_LIBDIR: ${CMAKE_INSTALL_LIBDIR})
message(CMAKE_INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${PROJECT_NAME}_LIB
        LIBRARY DESTINATION ${CMAKE_BINARY_DIR}
        DESTINATION ${CMAKE_BINARY_DIR}
)
