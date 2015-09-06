/**
 * @file Samples.h
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#ifndef SAMPLE_H_
#define SAMPLE_H_

#include <iostream>
#include "opencv2/opencv.hpp"

using namespace std;

class Sample
{
public:
	Sample(cv::Mat img,int identity = -1): mSampleImage(img),mIdentity(identity) {};
	virtual ~Sample() {
	};

	cv::Mat GetImage() {return mSampleImage;};
	int GetIdentity() {return mIdentity;};
	void SetIdentity(int id) {mIdentity = id;};

	bool AddMatrix(string key, cv::Mat matrix){
		if(mMatrices.find(key) != mMatrices.end()){
			return false;
		}
		mMatrices.insert(pair<string, cv::Mat>(key,matrix));
		return true;
	};

	bool ReplaceMatrix(string key, cv::Mat matrix){
		if(mMatrices.find(key) == mMatrices.end()){
			return false;
		}
	
		mMatrices[ key] = matrix;
		return true;
	}

	cv::Mat GetMatrix(string key){
		return mMatrices[key];
	};

	bool HasMatrix(string key){
		return mMatrices.find(key) != mMatrices.end();
	};

	map<string, cv::Mat> * GetMatricesMap() {return &mMatrices;};

private:
	cv::Mat					mSampleImage;
	int						mIdentity;
	map<string, cv::Mat>	mMatrices;
};


#endif //SAMPLE_H_