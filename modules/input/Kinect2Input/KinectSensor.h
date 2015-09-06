
#include <Kinect.h>
#include <Kinect.Face.h>
#include <opencv2/opencv.hpp>


class KinectSensor
{
public:
	static const int        cColorWidth = 1920;
	static const int        cColorHeight = 1080;
	
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;

	static const int        cBodyIndexWidth = 512;
	static const int        cBodyIndexHeight = 424;


	KinectSensor(bool detectFaces = true);
    ~KinectSensor();
	bool IsOpen();
	bool Open();

	void Capture();
	cv::Mat GetColorImage();
	cv::Mat GetDepthImage();
	cv::Mat GetBodyIndexImage();
	cv::Mat GetSkeletons();
	cv::Mat GetFaces();

private:
	IKinectSensor * m_pKinectSensor;
	IColorFrameReader * m_pColorFrameReader;
	IDepthFrameReader * m_pDepthFrameReader;
	IBodyIndexFrameReader * m_pBodyIndexFrameReader;
	IFaceFrameReader* m_pFaceReader[BODY_COUNT];
	IFaceFrameSource* m_pFaceSource[BODY_COUNT];

	IMultiSourceFrameReader *m_pFrameReader;
	
	cv::Mat m_Skeleton;
	cv::Mat m_Faces;

	RGBQUAD * m_pColorRGBX;
	UINT16 * m_pDepth;
	UINT8 * m_pBodyIndex;
	bool mInitialized;
	cv::Mat mColor;
	cv::Mat mDepth;
	cv::Mat mIndex;
	
};
