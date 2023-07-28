################################################
### BUILD instructions (generate binary in /src/build)
## cmake [options]
# -D BUILD_TESTS=ON/OFF
# -D SECURITY_FOLDER=/path/to/security/configs
#
# cmake -B [directory] -S [location of CMakeLists.txt] [options]
# mkdir -p && cd build
# cmake ..
# make -j -l5
### RUN instructions (run binary file `gstreamer` in /src/build)
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

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
    set(BUILD_ARCHITECTURE "x64" )
else(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "aarch64" )
    set(BUILD_ARCHITECTURE "arm64" )
    set(PLATFORM_TEGRA "TRUE")
    ADD_DEFINITIONS(-DPLATFORM_TEGRA=TRUE)
endif()

set(GTest_DIR /start/vcpkg/installed/${BUILD_ARCHITECTURE}-linux/share/gtest)

set("CMAKE_TOOLCHAIN_FILE /start/vcpkg/scripts/buildsystems/vcpkg.cmake")
include(/start/vcpkg/scripts/buildsystems/vcpkg.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_MODULES_DIR}")

# cmake .. -D BUILD_TESTS=ON
option(BUILD_TESTS "Build unit tests" OFF)
# cmake .. -D LOG_LEVEL=2 (0=INFO, 1=WARNING, 2=ERROR, 3=FATAL) <NOTE will not save .dot files unless=1>
option(GOOGLE_LOG_LEVEL "Set google log levels" OFF)
# cmake .. -D YAML_CONFIGS=ON
option(YAML_CONFIGS "Include YAML configs in Pipeline module" OFF)
# cmake .. -D ENABLE_DOT=ON
option(ENABLE_DOT "Create image from .dot when pipeline changes state" OFF)


if(GOOGLE_LOG_LEVEL)
    add_compile_definitions(GOOGLE_LOG_LEVEL="${GOOGLE_LOG_LEVEL}")
else()
    add_compile_definitions(GOOGLE_LOG_LEVEL=1)
endif()
message(STATUS "Set GLog level to ${GOOGLE_LOG_LEVEL}")

if(YAML_CONFIGS)
    add_compile_definitions(YAML_CONFIGS=TRUE)
    set(YAML_CONFIGS "TRUE")
    ADD_DEFINITIONS(-DYAML_CONFIGS=TRUE)
endif()

if(ENABLE_DOT)
    add_compile_definitions(ENABLE_DOT=TRUE)
    set(ENABLE_DOT "TRUE")
    ADD_DEFINITIONS(-DENABLE_DOT=TRUE)
endif()

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
add_subdirectory(modules/softwareLicense)


################################################
# Include libraries base cmakes for all modules
################################################

include(${CMAKE_MODULES_DIR}/date.cmake)
include(${CMAKE_MODULES_DIR}/glog.cmake)
include(${CMAKE_MODULES_DIR}/gstreamer.cmake)
include(${CMAKE_MODULES_DIR}/nlohmannjson.cmake)
include(${CMAKE_MODULES_DIR}/nvds.cmake)
include(${CMAKE_MODULES_DIR}/opencv.cmake)
include(${CMAKE_MODULES_DIR}/threadpool.cmake)
include(${CMAKE_MODULES_DIR}/uuid.cmake)
include(${CMAKE_MODULES_DIR}/yaml.cmake)

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
