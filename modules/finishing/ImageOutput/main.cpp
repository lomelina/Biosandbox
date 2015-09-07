#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/timeb.h>
#include "biosandbox/Modules.h"

using namespace cv;

unsigned long getMilliCount(){
	timeb tb;
	ftime(&tb);
	unsigned long nCount = tb.millitm;
	nCount += (tb.time & 0xfffff) * 1000;
	return nCount;
}

template<class T>
    std::string toString(const T& t)
{
     std::ostringstream stream;
     stream << t;
     return stream.str();
}

bool waitForKeyEachCycle = false;
bool showWindow = true;
bool saveImages = false;
bool initialized = false;
bool showDetectedIds = false;
std::string showDetectedTags = "";
std::string propertyToShow = "image";
std::string savePath = "";

bool saveOnlyWithTag = false;
std::string saveTag = "";

CvScalar * colors = new CvScalar[8];



char bufferTime [45];
char bufferId [10];
time_t lastTime;
int imageId = 0;
map<int, string> names;
void loadNames(string file) {
	int id;
	string name;

	ifstream infile(file);
	if(infile.is_open()){
		while (!infile.eof()) {
			infile >> id >> name;
			names[id] = name;
		}
		infile.close();
	} else {
		cerr << "Error: ImageOutput: unable to open namesFile: " << file <<  endl;
	}
}


class ImageOutput : public FinishingModule {
	public:
		ImageOutput();
		~ImageOutput();
		bool Finish(vector<Sample *> & rSamplesToProcess);

	private:

};

ImageOutput::ImageOutput(){
	lastTime = time(0);
}

ImageOutput::~ImageOutput(){

}

bool ImageOutput::Finish(vector<Sample *> & rSamplesToProcess){
	if(!initialized){
		if(mAttributes.find("showWindow") != mAttributes.end()){
			showWindow = mAttributes["showWindow"] != "false";	
		}
		if(mAttributes.find("waitForKeyEachCycle") != mAttributes.end()){
			waitForKeyEachCycle = mAttributes["waitForKeyEachCycle"] != "false";
		}
		if(mAttributes.find("savePath") != mAttributes.end()){
			saveImages = true;
			savePath = mAttributes["savePath"] ;
		}
		if(mAttributes.find("saveTag") != mAttributes.end()){
			saveOnlyWithTag = true;
			saveTag = mAttributes["saveTag"] ;
		}
		if(mAttributes.find("displayTag") != mAttributes.end()){
			showDetectedIds = true;
			showDetectedTags = mAttributes["displayTag"] ;
		}
		if(mAttributes.find("namesFile") != mAttributes.end()){
			loadNames(mAttributes["namesFile"]);
		}
		colors[0] = CV_RGB(255,0,0);
		colors[1] = CV_RGB(0,255,0);
		colors[2] = CV_RGB(0,0,255);
		colors[3] = CV_RGB(255,0,255);
		colors[4] = CV_RGB(255,255,0);
		colors[5] = CV_RGB(0,255,255);
		
		initialized = true;
	}

	if(rSamplesToProcess.size() > 0)
	{
		if(saveImages){
			int imgNum = 0;
			for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
				it!=rSamplesToProcess.end();
				it++) 
			{
				//if((*it)->HasMatrix("faces"))
				//{
				//	string fname = (string)((string)savePath+bufferTime + bufferId + ".orig.png");
				//	imwrite(fname,(*it)->GetMatrix("GrayImage"));
				//	Mat regions = (*it)->GetMatrix("faces");
				//	ofstream facesfile;
				//	facesfile.open (fname +".faces");
  
  
				//	for(int i = 0; i < regions.rows; i++)
				//	{
				//		Mat face, coords = regions.row(i).clone(); 
				//		//circle((*it)->GetImage(),Point(coords.at<unsigned short>(0,5),coords.at<unsigned short>(0, 6)),3, CV_RGB(255,255,255));
				//		//circle((*it)->GetImage(),Point(coords.at<unsigned short>(0,7),coords.at<unsigned short>(0, 8)),3, CV_RGB(255,255,255));
				//		for(int p=0; p< regions.cols;p++ ){
				//			facesfile << regions.at<unsigned short>(i,p) <<   ( p < (regions.cols -1)? " " : "");
				//		}
				//		facesfile << endl;
				//		
				//		
				//
				//		
				//	}
				//	facesfile.close();
				//}
				if(saveOnlyWithTag &&  !((*it)->HasMatrix(saveTag)) )
					continue;
				imgNum ++;
				time_t now = time(0);
				ltoa(now,bufferTime,10);
				imageId = lastTime == now? imageId+1 : 0;
				itoa(imageId,bufferId,10);
				string filename = (string)((string)savePath+bufferTime + bufferId + ".png");
				imwrite(filename,(*it)->GetImage());
				lastTime = now;

				
			}
		}
		if(showWindow) 
		{
			Mat img = rSamplesToProcess[0]->GetImage().clone();
			int colorId = 0;
			if(showDetectedIds){
				for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
					it!=rSamplesToProcess.end();
					it++) 
				{
					if((*it)->HasMatrix(showDetectedTags)){
						Mat r = (*it)->GetMatrix(showDetectedTags);
						if(names.find((*it)->GetIdentity()) != names.end()){
							//cout << (*it)->GetIdentity() << endl;
							cv::putText(img,names[(*it)->GetIdentity()],cv::Point(r.at<unsigned short>(0,0),r.at<unsigned short>(0,1)-10),FONT_HERSHEY_COMPLEX_SMALL, 1.8, colors[min(5,colorId)], 1);
						}
						else {
							ltoa((*it)->GetIdentity(),bufferTime,10);
							cv::putText(img,bufferTime,cv::Point(r.at<unsigned short>(0,0),r.at<unsigned short>(0,1)-10),FONT_HERSHEY_COMPLEX_SMALL, 1.8, CV_RGB(255,255,255), 1);
						}
						cv::rectangle(img,Rect(r.at<unsigned short>(0,0),r.at<unsigned short>(0,1),r.at<unsigned short>(0,2),r.at<unsigned short>(0,3)), colors[min(5,colorId++)], 3, 8, 0 );
					}
					
				}
			}

			//Mat m = img.clone();
			//string text = "TEXT";
			//cv::putText(m,text,cv::Point(100,100),FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(0,255,0), 1);
			//cv::rectangle(m,Rect(100,100,100,100), CV_RGB(0,255,0), 3, 8, 0 );
			unsigned long start = getMilliCount();
			imshow(propertyToShow, img);	
			unsigned long end = getMilliCount();
			cout << "imshow() time is " << (end - start) << endl;
			if(waitForKeyEachCycle){
				int key = waitKey();
				return (key == 27 ? true : false);
			} else {
				if(waitKey(1) >= 0) 
					return true;
			}
		}
	}

	return false;
}

extern "C" __declspec(dllexport) BaseModule * GetModuleFromFile() {
	return new ImageOutput();
}
