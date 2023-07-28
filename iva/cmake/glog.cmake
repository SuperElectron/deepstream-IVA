#
# AVAILABLE VARIABLES
# (not sure for includes)   # include dir

# set (GLOG_LDFLAGS)       # found libraries

set(MODULE_NAME "glog.cmake")
message(STATUS ${MODULE_NAME} [start] ----------------------)

find_package(glog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}_LIB PUBLIC glog::glog)

message(STATUS ${MODULE_NAME} [finish] ----------------------)