INCLUDE_DIRECTORIES(${BIOSANDBOX_SOURCE}/include ${OpenCV_INCLUDE_DIRS})

ADD_LIBRARY( FeretListInput SHARED main )
TARGET_LINK_LIBRARIES( FeretListInput opencv_core opencv_highgui )
SET_PROPERTY(TARGET FeretListInput PROPERTY FOLDER "input")

INSTALL(TARGETS FeretListInput DESTINATION bin/modules )

