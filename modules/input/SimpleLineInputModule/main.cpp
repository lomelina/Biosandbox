/**
 * @file SimpleLineModule/main.cpp
 *
 *
 *
 */
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "biosandbox/Modules.h"

INPUT_MODULE( SimpleLineInputModule)
//
//  instead of:
//
//class SimpleLineInputModule : public InputModule {
//		public:
//			SimpleLineInputModule(){};
//			~SimpleLineInputModule(){};
//			bool read(vector<Sample *> samplesToProcess);
//	};
//extern "C" __declspec(dllexport) BaseModule * GetModuleFromFile() {
//	return new SimpleLineInputModule();
//}


bool SimpleLineInputModule::Read(std::vector<Sample *> & rSamplesToProcess){
	
	if(mAttributes.find("file") == mAttributes.end()){
		cerr<<"Parameter file is not set!!!"<< endl;
		return false;
	}

	string line = "";
	int id; 

	ifstream infile(mAttributes["file"]);
	
	while (!infile.eof()) {
		infile >> id >> line;
		cv::Mat img = cv::imread(line, 0);
		if( img.data == 0 ){
			cerr<<"Unable to load image: "<< line <<endl;
			continue;
		}
		rSamplesToProcess.push_back( new Sample(img,id));
	}
	
	infile.close();
	
	return true;
}