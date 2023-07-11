set(MODULE_NAME "yaml.cmake")
message(STATUS ${MODULE_NAME} [start] ----------------------)

find_package(PkgConfig REQUIRED)
find_package(yaml-cpp REQUIRED)
target_link_libraries(${PROJECT_NAME}_LIB PRIVATE yaml-cpp)

message(STATUS ${MODULE_NAME} [finish] ----------------------)



