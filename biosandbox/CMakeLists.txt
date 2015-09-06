INCLUDE_DIRECTORIES( src ${TINYXML} ${OpenCV_INCLUDE_DIRS})

ADD_LIBRARY( tinyxml ${TINYXML}/tinyxml.cpp
${TINYXML}/tinyxmlerror.cpp ${TINYXML}/tinyxmlparser.cpp
${TINYXML}/tinystr.cpp )

ADD_EXECUTABLE( biosandbox src/main src/base/Configuration
src/base/WorkflowEngine src/conf/XmlConfigurationLoader
src/platform/${CMAKE_SYSTEM_NAME}/DynamicLoader )


FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/base/Modules.h
${CMAKE_CURRENT_SOURCE_DIR}/src/base/Sample.h DESTINATION
${CMAKE_CURRENT_SOURCE_DIR}/include/biosandbox)

if(WIN32)
	TARGET_LINK_LIBRARIES( biosandbox imagehlp tinyxml ${EXTERNAL_LIBS} )
else(WIN32)
	TARGET_LINK_LIBRARIES( biosandbox dl tinyxml ${EXTERNAL_LIBS} )
endif(WIN32)

INSTALL(FILES src/base/Modules.h src/base/Sample.h
DESTINATION include/biosandbox )

INSTALL(TARGETS biosandbox DESTINATION bin)
#INSTALL(FILES ${OpenCV_LIB_COMPONENTS} DESTINATION bin)

                 
