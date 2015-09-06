#include <iostream>
#include <fstream>
#include <stdio.h>
#include "biosandbox/Modules.h"

INPUT_MODULE( CameraInput )

using namespace cv;

bool cameraInitialized = false;
VideoCapture cap;

bool CameraInput::Read(std::vector<Sample *> & rSamplesToProcess){
	int device = 0;
	
	if(!cameraInitialized){
		if( mAttributes.find("device") == mAttributes.end()) {
			istringstream ( mAttributes["device"] ) >> device;
		} 
		cap.open(device);
		cameraInitialized = true;
	}
	
	Mat frame;
	cap >> frame;
	
	rSamplesToProcess.push_back( new Sample(frame));

	return false;
}