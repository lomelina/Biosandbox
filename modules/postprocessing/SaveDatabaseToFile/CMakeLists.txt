INCLUDE_DIRECTORIES(${BIOSANDBOX_SOURCE}/include ${OpenCV_INCLUDE_DIRS})

ADD_LIBRARY( SaveDatabaseToFile SHARED main )
TARGET_LINK_LIBRARIES( SaveDatabaseToFile opencv_core opencv_highgui )
SET_PROPERTY(TARGET SaveDatabaseToFile PROPERTY FOLDER "postprocessing")


INSTALL(TARGETS SaveDatabaseToFile DESTINATION bin/modules )

