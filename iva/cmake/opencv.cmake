#
# AVAILABLE VARIABLES
# set (OpenCV_LIBS)                # found libraries
# set (OpenCV_COMPONENTS_REQUIRED) # requested components
# set (OpenCV_LIB_COMPONENTS)      # found components
# set (OpenCV_VERSION)             # found version
set(MODULE_NAME "opencv.cmake")
message(STATUS ${MODULE_NAME} [start] ----------------------)

set(OpenCV_DIR /usr/include/opencv)

# similar to: OpenCV=`pkg-config --cflags --libs opencv4`
find_package(OpenCV 4
        #        EXACT
        REQUIRED
        COMPONENTS
        opencv_core
        opencv_highgui
        CONFIG
        )

message(STATUS
        "OpenCV_VERSION=(${OpenCV_VERSION})
        OpenCV_COMPONENTS_REQUIRED=(${OpenCV_COMPONENTS_REQUIRED})
        OpenCV_LIB_COMPONENTS=(${OpenCV_LIB_COMPONENTS})"
        )

target_include_directories(${PROJECT_NAME}_LIB PUBLIC ${OpenCV_DIR})
target_link_libraries(${PROJECT_NAME}_LIB PUBLIC ${OpenCV_LIBS})

message(STATUS ${MODULE_NAME} [finish] ----------------------)