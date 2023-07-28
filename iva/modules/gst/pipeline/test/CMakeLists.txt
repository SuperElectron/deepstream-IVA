# Note: gtest requires at least C++ 14

set(MODULE_NAME "test_gst.pipeline")
message(STATUS "*** building ${MODULE_NAME} module ***")

# add source files
file(GLOB_RECURSE TEST_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        )

# generate and link executables
add_executable(${MODULE_NAME} ${TEST_SRCS})
target_link_libraries(${MODULE_NAME} PUBLIC GTest::gtest GTest::gtest_main ${PROJECT_NAME}_LIB)

# automatic discovery of unit tests
gtest_discover_tests(
        ${MODULE_NAME}
        PROPERTIES
        LABELS "unit"
        DISCOVERY_TIMEOUT 30 # how long to wait (in seconds) before crashing
)

message(STATUS "*** finished ${MODULE_NAME} module ***")