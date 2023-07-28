#
# AVAILABLE VARIABLES
# set (GSTREAMER_INCLUDE_DIR)   # include dir
# set (GSTVIDEO_INCLUDE_DIR)    # include dir
# set (GLIB2_INCLUDE_DIR)       # include dir

# set (GSTREAMER_LDFLAGS)       # found libraries
# set (GSTVIDEO_LDFLAGS)       # found libraries
# set (GLIB2_LIBRARIES)       # found libraries
set(MODULE_NAME "gstreamer.cmake")
message(STATUS ${MODULE_NAME} [start] ----------------------)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER gstreamer-1.0>=1.16 REQUIRED)
pkg_check_modules(GSTVIDEO gstreamer-video-1.0>=1.16 REQUIRED)
pkg_check_modules(GSTAPP gstreamer-app-1.0>=1.16 REQUIRED)
pkg_check_modules(GSTUTILS gstreamer-pbutils-1.0>=1.16 REQUIRED)
pkg_check_modules(GLIB2 glib-2.0 REQUIRED)

#message(STATUS "***** GSTVIDEO DIR: ${GSTVIDEO_INCLUDE_DIRS}")
#message(STATUS "***** GSTREAMER_LIBRARIES: ${GSTREAMER_LIBRARIES}")
#message(STATUS "***** GSTREAMER_INCLUDE_DIRS: ${GSTREAMER_INCLUDE_DIRS}")
#message(STATUS "***** GSTREAMER_LDFLAGS: ${GSTREAMER_LDFLAGS}")

target_include_directories(${PROJECT_NAME}_LIB PUBLIC
        ${GLIB2_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${GSTVIDEO_INCLUDE_DIRS}
        ${GSTAPP_INCLUDE_DIRS}
        ${GSTUTILS_INCLUDE_DIRS}
        )

target_link_libraries(${PROJECT_NAME}_LIB PUBLIC
        ${GLIB2_LIBRARIES}
        ${GSTUTILS_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        ${GSTUTILS_LDFLAGS}
        ${GSTVIDEO_LDFLAGS}
        ${GSTAPP_LDFLAGS}
        ${GSTREAMER_LDFLAGS}
)

message(STATUS ${MODULE_NAME} [finish] ----------------------)