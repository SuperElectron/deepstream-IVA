#
# AVAILABLE VARIABLES
# set (nlohmann_json::nlohmann_json)                    # found libraries

set(MODULE_NAME "nlohmannjson.cmake")
message(STATUS ${MODULE_NAME} [start] ----------------------)

find_package(nlohmann_json 3.2.0 REQUIRED)
include_directories(/usr/include/)
target_link_libraries(${PROJECT_NAME}_LIB PUBLIC nlohmann_json::nlohmann_json)

message(STATUS ${MODULE_NAME} [finish] ----------------------)