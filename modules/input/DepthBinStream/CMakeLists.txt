INCLUDE_DIRECTORIES(${BIOSANDBOX_SOURCE}/include ${OpenCV_INCLUDE_DIRS})


ADD_LIBRARY( DepthBinStream SHARED DepthBinStream)
TARGET_LINK_LIBRARIES( DepthBinStream opencv_core opencv_highgui )
SET_PROPERTY(TARGET DepthBinStream PROPERTY FOLDER "input")
INSTALL(TARGETS DepthBinStream DESTINATION bin/modules )

