#include <iostream>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


#include "biosandbox/Modules.h"


PREPROCESSING_MODULE(Splitter)

using namespace cv;

int width = -1;
int height = -1;


Mat preprocessImage(Mat image, unsigned short *landmarks){
	Point2f srcTri[3], dstTri[3];
	//cout <<"Landmarks" <<  landmarks[0 ] << ";"<< landmarks[1 ] << ";"<< landmarks[2 ] << ";"<< landmarks[3 ] << ";" << endl;
	int midx = (landmarks[0 ] + landmarks[2 ])/2;
	int midy = (landmarks[1] + landmarks[3])/2;
	srcTri[0].x = landmarks[0];
	srcTri[0].y = landmarks[1];
	srcTri[1].x = landmarks[2];
	srcTri[1].y = landmarks[3];
	srcTri[2].x = midx + landmarks[1] - landmarks[3];
	srcTri[2].y = midy + landmarks[2] - landmarks[0];

//	dstTri[0].x = 50+(width/4);//80;
//	dstTri[0].y = 90+(height/3);//160;
//	dstTri[1].x = 50+((width/4)*3);//140;
//	dstTri[1].y = 90+(height/3);//160;
//	dstTri[2].x = 50+((width/4)*2);//110;
//	dstTri[2].y = 90+(height/3) + (width/2);//160+60;

	dstTri[0].x = (width/4);//80;
	dstTri[0].y = (height/3);//160;
	dstTri[1].x = ((width/4)*3);//140;
	dstTri[1].y = (height/3);//160;
	dstTri[2].x = ((width/4)*2);//110;
	dstTri[2].y = (height/3) + (width/2);//160+60;


//	Mat temp = image.clone();
//	Mat warp_mat = cv::getAffineTransform( srcTri, dstTri );
//	warpAffine( image, temp, warp_mat,cvSize(image.cols,image.rows),0 );
//	temp = temp(Rect(50,90,width,height));

	Mat temp(height,width,image.type());
	Mat warp_mat = cv::getAffineTransform( srcTri, dstTri );
	warpAffine( image, temp, warp_mat,cvSize(width,height),0 );

	equalizeHist(temp,temp);
	return temp;
}


vector<Sample *> samples;
string regionsTag = "faces";
string regionTag = "face";

bool initialized = false;

bool Splitter::Process(vector<Sample *> & rSamplesToProcess) 
{
	if(!initialized){
		if(mAttributes.find("width") != mAttributes.end()){
			istringstream convert(mAttributes["width"]);
			convert >> width;
		}
		if(mAttributes.find("height") != mAttributes.end()){
			istringstream convert(mAttributes["height"]);
			convert >> height;
		}

		if(width == -1 || height == -1){
			cout << "Splitter: Using width = 120 and height = 180" << endl;
			width = 120;
			height = 180;
		}
		initialized = true;

	}

	samples.clear();
	samples = rSamplesToProcess;
	//rSamplesToProcess.clear(); // in the beginning, I  thinking about replacing the samples

	for(	vector<Sample *>::iterator it = samples.begin();
			it!=samples.end();
			it++) 
	{	
		Mat gray_image;
		if(!(*it)->HasMatrix("GrayImage")){
			cvtColor((*it)->GetImage(), gray_image, COLOR_BGR2GRAY );
		//	imshow("GRAY",gray_image);
			(*it)->AddMatrix("GrayImage",gray_image);
		} else {
			gray_image = (*it)->GetMatrix("GrayImage");
		}
		if((*it)->HasMatrix(regionsTag))
		{
			Mat regions = (*it)->GetMatrix(regionsTag);
			for(int i = 0; i < regions.rows; i++)
			{
				Mat face, coords = regions.row(i).clone(); // without clone() access violation occures - perhaps bug in OpenCV or Windows
				if (coords.at<unsigned short>(0,2) == 0 && coords.at<unsigned short>(0,3) == 0) {
					continue;
				} 
				
/*				resize(gray_image(Rect(coords.at<unsigned short>(0,0),
								coords.at<unsigned short>(0,1),
								coords.at<unsigned short>(0,2),
								coords.at<unsigned short>(0,3))),face,
								Size(
									width>0?width:coords.at<unsigned short>(0,2),
									height>0?height:coords.at<unsigned short>(0,3)));
				cv::equalizeHist(face,face);
*/			
				//int pointsCount = (coords.cols - 5) /2;
				//for(int p = 0; p < pointsCount; p++){
				//	circle((*it)->GetImage(),Point(coords.at<unsigned short>(0,p+5),coords.at<unsigned short>(0,p + 6)),3, CV_RGB(255,0,0));
				//}
				//circle((*it)->GetImage(),Point(coords.at<unsigned short>(0,5),coords.at<unsigned short>(0, 6)),3, CV_RGB(255,255,255));
				//circle((*it)->GetImage(),Point(coords.at<unsigned short>(0,7),coords.at<unsigned short>(0, 8)),3, CV_RGB(255,255,255));
				//imshow("hahah",gray_image);
				
				Mat f =  preprocessImage(gray_image,(unsigned short *)(coords.data + 10));

				Sample * newSample = new Sample(f,coords.cols >= 5 ? coords.at<unsigned short>(0,4) : 0);
				newSample->AddMatrix(regionTag,coords);
				rSamplesToProcess.push_back(newSample);
#ifdef DEBUG
				char faceName[1024];
				sprintf(faceName,"%d",newSample->GetIdentity());
				
				imshow(faceName,f);
#endif	
			}
		}

		
	}
	

	return true;
}