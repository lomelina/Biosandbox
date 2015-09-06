
// we are using MS driver for kinect so we need to include windows libraries as well
#include <windows.h>  
#include "KinectSensor.h"
//#include "FTHelper2.h"


#include <iostream>
#include <stdio.h>
#include "biosandbox/Modules.h"

INPUT_MODULE( Kinect2Input )

using namespace cv;
//
//int getWidth(NUI_IMAGE_RESOLUTION res){
//	switch(res){
//		case NUI_IMAGE_RESOLUTION_80x60:
//			return 80;
//		case NUI_IMAGE_RESOLUTION_320x240:
//			return 320;
//		case NUI_IMAGE_RESOLUTION_640x480:
//			return 640;
//		case NUI_IMAGE_RESOLUTION_1280x960:
//			return 1280;
//		default:
//			return 640;
//	}
//}
//
//int getHeight(NUI_IMAGE_RESOLUTION res){
//	switch(res){
//		case NUI_IMAGE_RESOLUTION_80x60:
//			return 60;
//		case NUI_IMAGE_RESOLUTION_320x240:
//			return 240;
//		case NUI_IMAGE_RESOLUTION_640x480:
//			return 480;
//		case NUI_IMAGE_RESOLUTION_1280x960:
//			return 960;
//		default:
//			return 480;
//	}
//}
//
//bool trackFaces = false;
KinectSensor * sensor = NULL;
//FTHelper2  tracker;
bool cameraInitialized = false;
//bool useDepth = false;
string depthTag = "depthImage";
string bodyIndexTag = "bodyIndex";
//
//NUI_IMAGE_RESOLUTION getResolution(string resolution){
//	if(resolution == "80x60"){
//		return NUI_IMAGE_RESOLUTION_80x60;
//	} else if(resolution == "320x240"){
//		return NUI_IMAGE_RESOLUTION_320x240;
//	} else if(resolution == "640x480"){
//		return NUI_IMAGE_RESOLUTION_640x480;
//	} else if(resolution == "1280x960"){
//		return NUI_IMAGE_RESOLUTION_1280x960;
//	} 
//	cerr << "Error: KinectInput: Unable to parse resolution from " << resolution << ". Using 640x480 ..." <<endl;
//	cerr << "\t Valid resolutions are: 80x60, 320x240, 640x480 and 1280x960"  << endl;
//	return NUI_IMAGE_RESOLUTION_640x480;
//};

bool Kinect2Input::Read(std::vector<Sample *> & rSamplesToProcess){
	

	if(!sensor || !sensor->IsOpen()){
		if (!sensor) sensor = new KinectSensor();
		sensor->Open();
		//NUI_IMAGE_RESOLUTION depthSize = NUI_IMAGE_RESOLUTION_320x240;
		//NUI_IMAGE_RESOLUTION colorSize = NUI_IMAGE_RESOLUTION_640x480;
		//int sensor = 0;
		//if( mAttributes.find("sensorId") != mAttributes.end()) {
		//	istringstream ( mAttributes["sensorId"] ) >> sensor;
		//} 
		//if( mAttributes.find("depthSize") != mAttributes.end()) {
		//	depthSize = (getResolution(mAttributes["depthSize"]));
		//	useDepth = true;
		//} 
		//if( mAttributes.find("colorSize") != mAttributes.end()) {
		//	colorSize = (getResolution(mAttributes["colorSize"]));
		//} 

		//if( mAttributes.find("trackFaces") != mAttributes.end()) {
		//	trackFaces = true;
		//} 

		////tracker.Init(6,NULL,NULL,NULL,NULL,NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,NUI_IMAGE_RESOLUTION_320x240,0,NUI_IMAGE_TYPE_COLOR,NUI_IMAGE_RESOLUTION_640x480,0);
		//tracker.Init(6,NULL,NULL,NULL,NULL,NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,depthSize,0,NUI_IMAGE_TYPE_COLOR,colorSize,0);
		
		cameraInitialized = true;
	}
	if (sensor && sensor->IsOpen()){
		sensor->Capture();
		Mat colorImage = sensor->GetColorImage();
		Mat depthImage = sensor->GetDepthImage();
		Mat bodyIndexImage = sensor->GetBodyIndexImage();
		
		Sample * sample = new Sample(colorImage);
		sample->AddMatrix(depthTag, depthImage);
		sample->AddMatrix(bodyIndexTag, bodyIndexImage);
		sample->AddMatrix("skeletons", sensor->GetSkeletons());
		sample->AddMatrix("faces", sensor->GetFaces());
		rSamplesToProcess.push_back(sample);
	}
	//while (!tracker.IsColorImageReady()) Sleep(100);

	////if(device != NULL){
	//	Mat colorImage = tracker.GetOpenCVColorImage();
	//	Mat depthImage;
	//	Sample * sample = new Sample(colorImage);
	//	sample->AddMatrix("skeletons", tracker.GetSkeletons());
	//	if(useDepth) {
	//		while (!tracker.IsDepthImageReady()) Sleep(100);
	//		depthImage =tracker.GetOpenCVDepthImage();
	//		sample->AddMatrix("ids",tracker.GetIds());
	//		sample->AddMatrix(depthTag,depthImage);
	//	}
	//	rSamplesToProcess.push_back( sample );

	//	if(trackFaces && useDepth) // TODO
	//	{
	//		Mat faces = tracker.GetFaces();// device->TrackFaces();
	//		if(faces.rows > 0){
	//			sample->AddMatrix("faces",faces);
	//		}
	//	}
	//}
	return false;
}