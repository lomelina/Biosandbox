#include "base/Modules.h"

typedef struct Coordinates2D
{
    int X;
    int Y;
    int Width;
    int Height;
	int TrackingId;
	int PlayerIndex;
	int LeftEyeX;
    int LeftEyeY;
	int RightEyeX;
    int RightEyeY;
}	Coordinates2D;


class InternalInput: public InputModule{
public: 
	InternalInput();
	bool Read(vector<Sample *> & rSamplesToProcess);

	void SetImage(char * imageData, int width, int height, Coordinates2D * coordinates, int nCoordinates);
	void SetDepthMap(unsigned short * depthPixelData,int depthWidth,int depthHeight);
private:
	Coordinates2D * mCoordinates;
	int mNumberOfCoordinates;
	char * mImageData;
	unsigned short * mDepthMapData;
	int mWidth;
	int mHeight;

	int mDepthWidth;
	int mDepthHeight;
};


class InternalOutput : public FinishingModule{
public: 
	InternalOutput();
	
	int GetIdentities(int * ids);
	int GetDistances(double * distances, int numDists);

	bool IsInteracting() {
		return mInteracting > 0;}

	bool Finish(vector<Sample *> & rSamplesToProcess);
private:
	int * mIdentities;
	int mNumIdentities;
	int mInteracting;
	vector<int> identities;
	vector<cv::Mat> distances;
};