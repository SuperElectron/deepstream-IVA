cmake_minimum_required(VERSION 3.15.2)

set(MODULE_NAME "GST")
message(STATUS "*** building ${MODULE_NAME} module ***")

add_subdirectory(pipeline)
add_subdirectory(processing)

message(STATUS "*** building ${MODULE_NAME} module ***")