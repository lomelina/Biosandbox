INCLUDE_DIRECTORIES(${BIOSANDBOX_SOURCE}/include ${OpenCV_INCLUDE_DIRS})


ADD_LIBRARY( SimpleLineInputModule SHARED main )
TARGET_LINK_LIBRARIES( SimpleLineInputModule opencv_core opencv_highgui )
SET_PROPERTY(TARGET SimpleLineInputModule PROPERTY FOLDER "input")

INSTALL(TARGETS SimpleLineInputModule DESTINATION bin/modules )

