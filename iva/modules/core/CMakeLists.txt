cmake_minimum_required(VERSION 3.15.2)

set(MODULE_NAME "CORE")
message(STATUS "*** building ${MODULE_NAME} module ***")

add_subdirectory(common)
add_subdirectory(event)
add_subdirectory(base)

message(STATUS "*** building ${MODULE_NAME} module ***")