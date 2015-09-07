#include "opencv2/opencv.hpp"
#include "biosandbox/Modules.h"

unsigned long getMilliCount();

class TemporalFilter
{
public:
	TemporalFilter(int trackingId, double treshold);
	~TemporalFilter();

	void Update(Sample * sample);

	void NotifyTrackedSkeletons(cv::Mat allSkeletons);

	int GetTrackingId() { return mTrackingId; }

	int GetPersonId() { return mClosestId; }

	unsigned long GetLastNotification() { return mLastNotification; }
	void SetLastNotification(unsigned long time) { mLastNotification = time; }


	/*
		GetLastHit returns time when
	*/
	unsigned long GetLastHit() { return mLastHit; }

	void SetFaceRegion(cv::Rect r) { mLastRectangle = r; }
	cv::Rect GetFaceRegion() { return mLastRectangle; }

	bool ShouldTrigger() {
		return mTrigger; 
	}

private:
	int GetClosestId(Sample * sample, double * distance);
	int EvaluateHistory();

	cv::Rect mLastRectangle;

	int mTrackingId;
	double mTreshold;

	unsigned long mLastHit;

	int pos;
	int ids[20];

	double mClosestDist;
	int mClosestId;

	unsigned long mLastNotification;

	bool mTrigger;
};


