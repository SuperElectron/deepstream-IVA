set(MODULE_NAME "date.cmake")
message(STATUS ${MODULE_NAME} [start] ----------------------)

set(date_DIR /start/vcpkg/packages/date_${BUILD_ARCHITECTURE}-linux/share/date)
find_package(PkgConfig REQUIRED)
find_package(date CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}_LIB PRIVATE date::date date::date-tz)

#pkg_check_modules(TIMEZONE_DATE
#        date
#        REQUIRED
#        )

#target_include_directories(${PROJECT_NAME}_LIB PUBLIC
#        date::date date::date-tz
#)

message(STATUS ${MODULE_NAME} [finish] ----------------------)



