INCLUDE_DIRECTORIES( ${BIOSANDBOX_SOURCE}/src ${TINYXML} ${OpenCV_INCLUDE_DIRS})
ADD_LIBRARY( libBiosandbox SHARED ${BIOSANDBOX_SOURCE}/src/main ${BIOSANDBOX_SOURCE}/src/base/Configuration
${BIOSANDBOX_SOURCE}/src/base/WorkflowEngine ${BIOSANDBOX_SOURCE}/src/conf/XmlConfigurationLoader
${BIOSANDBOX_SOURCE}/src/platform/${CMAKE_SYSTEM_NAME}/DynamicLoader c_interface InternalIO.cpp IterativeWorkflowEngine.cpp)

if(WIN32)
	TARGET_LINK_LIBRARIES( libBiosandbox imagehlp tinyxml ${EXTERNAL_LIBS} opencv_core opencv_highgui )
else(WIN32)
	TARGET_LINK_LIBRARIES( libBiosandbox dl tinyxml ${EXTERNAL_LIBS} opencv_core opencv_highgui )
endif(WIN32)

SET_PROPERTY(TARGET libBiosandbox PROPERTY FOLDER "library")

INSTALL(TARGETS libBiosandbox DESTINATION lib)
           
      
include_external_msproject(
    libBioSandbox_managed ../libBioSandbox_managed/libBioSandbox_managed.csproj
    TYPE 172C97E7-5279-4491-A728-D9FBBEECF2A2)

SET_PROPERTY(TARGET libBioSandbox_managed PROPERTY FOLDER "library")
