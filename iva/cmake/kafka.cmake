set(MODULE_NAME "kafka.cmake")
message(STATUS ${MODULE_NAME} [start] ----------------------)

# Include librdkafka C files
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(MESSAGING librdkafka REQUIRED)

find_package(RdKafka CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}_LIB PUBLIC RdKafka::rdkafka RdKafka::rdkafka++)


message(STATUS ${MODULE_NAME} [finish] ----------------------)