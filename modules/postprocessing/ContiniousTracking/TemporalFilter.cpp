#include <iostream>
#include <limits>
#include "TemporalFilter.h"
#include <sys/timeb.h>

using namespace std;
using namespace cv;

unsigned long getMilliCount(){
	timeb tb;
	ftime(&tb);
	unsigned long nCount = tb.millitm;
	nCount += (tb.time & 0xfffff) * 1000;
	return nCount;
}

TemporalFilter::TemporalFilter(int trackingId, double treshold) : 
	mTrackingId(trackingId), mTreshold(treshold)
{
	pos = 0;

	mClosestDist = numeric_limits<double>::max();
	mClosestId = -1;
	cout << "Init. temporal filter for tracking id: "  << trackingId << endl;
	mLastHit = getMilliCount();
	mTrigger = false;
	mLastNotification = 0;
	// initialize array
	for(int i = 0; i < 20; i++)
	{
		ids[i] = -1;
	}
}


TemporalFilter::~TemporalFilter(void)
{

}

int TemporalFilter::GetClosestId(Sample * sample, double * distance){
	double dist;
	Point loc;
	Mat m = sample->GetMatrix("differences");
	Mat ids = sample->GetMatrix("identities");
	
	minMaxLoc(m,&dist,0,&loc);
	int id = ids.at<int>(loc.x,0);

#ifdef DEBUG
	cout << "TrackingID: " << sample->GetIdentity() << endl;
	
	for(int i = 0; i < m.cols;i++){
		cout << m.at<float>(0,i) << ";";
	}
	cout<< endl;
	cout << "ID: " << id << "  dist = " << dist << endl;
#endif
	if(dist < mTreshold ){
		(*distance) = dist;
		return id;
	} 
	return -1;
}

int TemporalFilter::EvaluateHistory(){
	int histogram[20];
	memset(histogram,0,sizeof(int)*20);

	for(int i = 0; i < 20; i++)
	{
	//	cout << ids[i] << ";";
		if(ids[i] >= 0){
			
			histogram[ids[i]]++;
		}
	}
	//cout << endl;

	int n = -1,val = 0;
	for(int i = 0; i < 20; i++)
	{
		if(histogram[i] >= val){
			n = i;
			val = histogram[i];
		}
	}
	//cout << "Hits: " << val << "; ID: "  << n << endl;
	return val > 2 ? n : -1;
}

void TemporalFilter::NotifyTrackedSkeletons(Mat allSkeletons){
	for(int i = 0; i < allSkeletons.cols; i++){
		if(mTrackingId == allSkeletons.at<unsigned short>(i,0)){
			mLastHit = getMilliCount();
			return;
		}
	}
}

void TemporalFilter::Update(Sample * sample){
	if(sample->GetIdentity() != mTrackingId){
		cerr << "ERROR: Temporal filter (" << mTrackingId << ") received id = " << sample->GetIdentity() << endl;
		return;
	}
	double dist = numeric_limits<double>::max();
	int id = GetClosestId(sample,&dist);
	ids[pos++] = id;
	pos = pos % 20;

	int filteredId = EvaluateHistory();
	if(id != -1 && id == filteredId && mClosestDist > dist){
		mClosestDist = dist;

		if(mClosestId != filteredId)
			cout << "User Found/Update! TrackingId " << mTrackingId << " is now " <<  filteredId << endl;
		mTrigger = mClosestId != filteredId;

#ifdef DEBUG
		cout << "Updating Closest ID: " << mClosestId << endl;
#endif
		mClosestId = filteredId;
	}
#ifdef DEBUG
		cout << "Closest ID: " << mClosestId << "   Dist: " << mClosestDist << " Tracking ID: "<< mTrackingId << endl;
#endif
	sample->SetIdentity(mClosestId);
}