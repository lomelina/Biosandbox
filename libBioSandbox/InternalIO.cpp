#include <iostream>
#include <fstream>
#include <stdio.h>
#include "InternalIO.h"
#include "opencv2/opencv.hpp"


using namespace cv;
//----------------------------------------------------------------------
//
//	INTERNAL INPUT 
//
//----------------------------------------------------------------------

InternalInput::InternalInput() : InputModule(){
	mImageData = 0;
	mDepthMapData = 0;
	mWidth = 0;
	mHeight = 0;
}
	
void InternalInput::SetImage(char * imageData, int width, int height,Coordinates2D * coordinates, int nCoordinates){
	if(mImageData == 0){
		mImageData = imageData;
		mWidth = width;
		mHeight = height;
		mCoordinates = coordinates;
		mNumberOfCoordinates = nCoordinates;
	}
}

void InternalInput::SetDepthMap(unsigned short * depthPixelData,int depthWidth,int depthHeight){
	if(mDepthMapData == 0){
		mDepthMapData = depthPixelData;
		mDepthHeight = depthHeight;
		mDepthWidth = depthWidth;
	}
}

bool InternalInput::Read(std::vector<Sample *> & rSamplesToProcess){
	if(mImageData != 0){
		Mat image(mHeight,mWidth, CV_8UC4,(void *)mImageData);
		
		
		//cvtColor( image, image, CV_BGR2GRAY );
		//imshow("Test",image);
		//waitKey(1);

		Sample * sample = new Sample(image);
		Mat ids = Mat::zeros(1,6,CV_16UC1);
		
		Mat detectedFaces(mNumberOfCoordinates,9,CV_16UC1);
		//cout << mNumberOfCoordinates << endl;
		for( int i = 0; i < mNumberOfCoordinates; i++ )
		{
			detectedFaces.at<unsigned short>(i,0) = mCoordinates[i].X;
			detectedFaces.at<unsigned short>(i,1) = mCoordinates[i].Y;
			detectedFaces.at<unsigned short>(i,2) = mCoordinates[i].Width;
			detectedFaces.at<unsigned short>(i,3) = mCoordinates[i].Height;
			detectedFaces.at<unsigned short>(i,4) = mCoordinates[i].TrackingId;
			
			detectedFaces.at<unsigned short>(i,5) = mCoordinates[i].RightEyeX;
			detectedFaces.at<unsigned short>(i,6) = mCoordinates[i].RightEyeY;
			detectedFaces.at<unsigned short>(i,7) = mCoordinates[i].LeftEyeX;
			detectedFaces.at<unsigned short>(i,8) = mCoordinates[i].LeftEyeY;
			ids.at<unsigned short>(0,i) = mCoordinates[i].PlayerIndex;
			
		}
		sample->AddMatrix("faces",detectedFaces);
		sample->AddMatrix("ids",ids);
		
		if(mDepthMapData != 0){
			Mat depth(mDepthHeight,mDepthWidth, CV_16UC1,(void *)mDepthMapData);
			sample->AddMatrix("depthImage",depth);
		}

		
		rSamplesToProcess.push_back(sample);
		
		mImageData = 0;
	}
	return false;
}





//----------------------------------------------------------------------
//
//	INTERNAL OUTPUT 
//
//----------------------------------------------------------------------

InternalOutput::InternalOutput() : FinishingModule(){
	mIdentities = 0;
	mNumIdentities = 0;
	mInteracting = 0;
}

bool InternalOutput::Finish(std::vector<Sample *> & rSamplesToProcess){
	//int rightClassified = 0;
	identities.clear();
	distances.clear();
	for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
			it != rSamplesToProcess.end();
			it++){
			if((*it)->HasMatrix("face")){
				identities.push_back((*it)->GetIdentity());
			}
			if((*it)->HasMatrix("differences")){
				Mat m = (*it)->GetMatrix("differences");
				//for(int i = 0; i < m.cols;i++){
				//	cout << "Finishing: "<< m.at<float>(0,i) << ";";
				//}
				//cout<< endl;
				distances.push_back(m);
			}
	}
	if(rSamplesToProcess.size() > 0){
		mInteracting = rSamplesToProcess[0]->GetIdentity();
	}
	return false;
}

int InternalOutput::GetIdentities(int * ids){
	if(identities.size() > 0){ 
		memcpy(ids,&identities[0],sizeof(int)*identities.size());
		//cout << "Output " << identities[0] << endl;
		return identities.size();
	}
	ids = 0;
	return 0;
}

int InternalOutput::GetDistances(double * distances, int numDists){
	if(this->distances.size() > 0){
		for(int i = 0; i < this->distances.size();i++){
			Mat dists = this->distances[i];
			int count = __min(numDists,dists.cols);
			for(int d = 0; d < count; d++){
				distances[numDists*i + d] = dists.at<float>(0,d);
			}
		}
		return identities.size();
	}
	
	return 0;

}