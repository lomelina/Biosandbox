INCLUDE_DIRECTORIES(${BIOSANDBOX_SOURCE}/include ${OpenCV_INCLUDE_DIRS})


ADD_LIBRARY( ContiniousTracking SHARED main TemporalFilter )
TARGET_LINK_LIBRARIES( ContiniousTracking ${OpenCV_LIBS} )
SET_PROPERTY(TARGET ContiniousTracking PROPERTY FOLDER "postprocessing")


INSTALL(TARGETS ContiniousTracking DESTINATION bin/modules )

