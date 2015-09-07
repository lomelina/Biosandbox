INCLUDE_DIRECTORIES(${BIOSANDBOX_SOURCE}/include ${OpenCV_INCLUDE_DIRS})

ADD_LIBRARY( ChiSquareDistanceMatching SHARED main )
TARGET_LINK_LIBRARIES( ChiSquareDistanceMatching ${OpenCV_LIBS} )
SET_PROPERTY(TARGET ChiSquareDistanceMatching PROPERTY FOLDER "postprocessing")


INSTALL(TARGETS ChiSquareDistanceMatching DESTINATION bin/modules )

