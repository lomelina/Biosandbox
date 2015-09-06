#include <iostream>
#include <fstream>
#include <stdio.h>
#include "biosandbox/Modules.h"

using namespace cv;

//INPUT_MODULE( FeretListInput )

class FeretListInput : public InputModule {
public:
	FeretListInput() : initialized(false), continious(false) {};
	~FeretListInput(){};
	bool Read(vector<Sample *> & rSamplesToProcess);
private:
	bool initialized;
	bool continious;
	ifstream infile;
};
INTERN_LIBRARY_INTERFACE( FeretListInput )




string getFileName( string fullPath){
	int pos = fullPath.find_last_of("\\/");
	return pos == string::npos ? fullPath : fullPath.substr(pos+1);
}

Sample * readImage(ifstream & in){
	string line = "";
	int id; 
	in >> line;
	if (line.length() == 0) return 0;
	string fileName = getFileName(line);
	istringstream ( fileName.substr(0,5) ) >> id;
	Mat img = imread(line, 0);
	if( img.data == 0 ){
		cerr<<"Unable to load image: "<< line <<endl;
		return 0;
	}
	
	Sample * newSample = new Sample(img,id);
	newSample->AddMatrix("GrayImage",img);
	Mat name = Mat::zeros(1, 1024, CV_8UC1);
	
	memcpy(name.data, fileName.c_str(), fileName.length());

	ifstream facesFile(line + ".faces", std::ios::binary);
    if (facesFile)
    {
         Mat faces(1,9,CV_16SC1);
		 int num;
		 for(int i = 0; i < 9; i++){
			 facesFile >> num;
			 faces.at<unsigned short>(0,i) = num;
		 }
		 newSample->AddMatrix("faces",faces);
    }

	return newSample;
}

bool FeretListInput::Read(std::vector<Sample *> & rSamplesToProcess){
	if(!initialized){
		if(mAttributes.find("file") == mAttributes.end()){
			cerr<<"Parameter file is not set!!!"<< endl;
			return true;
		}
		if(mAttributes.find("continious") != mAttributes.end()){
			continious = true;
		}	
		
		infile.open(mAttributes["file"]);
		if(!infile.is_open()){
			cerr<<"File "<< mAttributes["file"] << " does not exist or it's not allowed to read" << endl;
			return true;
		}
		initialized = true;
	}
	
	if(!continious){
		while (!infile.eof()) {
			Sample * s = readImage(infile);
			if(s != 0) {
				rSamplesToProcess.push_back(s);
			}
		}
		infile.close();
		return true;
	} else {
		Sample * s = readImage(infile);
		bool ret = true;
		if(s != 0) {
			rSamplesToProcess.push_back(s);
		}
		
		if (ret = !infile.eof()) {
			infile.close();
		}
		return ret;
	}
}