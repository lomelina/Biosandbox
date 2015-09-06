
#include "KinectSensor.h"
#include <iostream>
#include <sys/timeb.h>

#define _USE_MATH_DEFINES
#include <math.h>

unsigned long getMilliCount(){
	timeb tb;
	ftime(&tb);
	unsigned long nCount = tb.millitm;
	nCount += (tb.time & 0xfffff) * 1000;
	return nCount;
}

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

// Quote from Kinect for Windows SDK v2.0 Developer Preview - Samples/Native/FaceBasics-D2D, and Partial Modification
// ExtractFaceRotationInDegrees is: Copyright (c) Microsoft Corporation. All rights reserved.
inline void ExtractFaceRotationInDegrees(const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll)
{
	double x = pQuaternion->x;
	double y = pQuaternion->y;
	double z = pQuaternion->z;
	double w = pQuaternion->w;

	// convert face rotation quaternion to Euler angles in degrees
	*pPitch = static_cast<int>(std::atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z) / M_PI * 180.0f);
	*pYaw = static_cast<int>(std::asin(2 * (w * y - x * z)) / M_PI * 180.0f);
	*pRoll = static_cast<int>(std::atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z) / M_PI * 180.0f);
}

KinectSensor::KinectSensor(bool detectFaces)
{
	m_pKinectSensor = 0;
	mInitialized = false;
	m_pColorRGBX  = new RGBQUAD[cColorWidth * cColorHeight];
	m_pDepth = new UINT16[cDepthWidth * cDepthHeight];
	m_pBodyIndex = new UINT8[cBodyIndexWidth * cBodyIndexHeight];
	m_Faces = cv::Mat::zeros(BODY_COUNT, 9, CV_16UC1);
}

KinectSensor::~KinectSensor()
{
	delete[] m_pColorRGBX;
	delete[] m_pDepth;
	delete[] m_pBodyIndex;
}

bool KinectSensor::IsOpen(){
	return mInitialized;
}


bool KinectSensor::Open(){
	HRESULT hr;
	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		std::cerr << "Kinect2Input: Unable to fetch installed sensor" << std::endl;
		return false;
	}
	if (m_pKinectSensor)
	{
		hr = m_pKinectSensor->Open();
		if (SUCCEEDED(hr)){
			m_pKinectSensor->OpenMultiSourceFrameReader(
				FrameSourceTypes::FrameSourceTypes_Color |
				FrameSourceTypes::FrameSourceTypes_Depth |
				FrameSourceTypes::FrameSourceTypes_Body |
				FrameSourceTypes::FrameSourceTypes_BodyIndex,
				&m_pFrameReader);
		}
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		std::cerr << "No ready Kinect found!" << std::endl;
		return false;
	}

	// initialize face detector
	m_pFaceSource[BODY_COUNT];
	DWORD features = FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
		| FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
		| FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
		| FaceFrameFeatures::FaceFrameFeatures_Happy
		| FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
		| FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
		| FaceFrameFeatures::FaceFrameFeatures_MouthOpen
		| FaceFrameFeatures::FaceFrameFeatures_MouthMoved
		| FaceFrameFeatures::FaceFrameFeatures_LookingAway
		| FaceFrameFeatures::FaceFrameFeatures_Glasses
		| FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;
	for (int count = 0; count < BODY_COUNT; count++){
		// Source
		hr = CreateFaceFrameSource(m_pKinectSensor, 0, features, &m_pFaceSource[count]);
		if (FAILED(hr)){
			std::cerr << "Error : Kinect2Input:KinectSensor.Open(): CreateFaceFrameSource" << std::endl;
			return false;
		}

		// Reader
		hr = m_pFaceSource[count]->OpenReader(&m_pFaceReader[count]);
		if (FAILED(hr)){
			std::cerr << "Error : Kinect2Input:KinectSensor.Open(): IFaceFrameSource::OpenReader()" << std::endl;
			return false;
		}
	}


	mInitialized = true;
	return true;
}


void KinectSensor::Capture(){
	if (!m_pColorFrameReader || !m_pDepthFrameReader || !m_pBodyIndexFrameReader)
	{
		std::cerr << "Error: Unable to capture frames because readers are not initialized" << std::endl;
		return;
	}

	INT64 timeStampBodyIndex = 0L;
	INT64 timeStampBody = 0L;
	INT64 timeStampColor = 0L;
	INT64 timeStampDepth = 0L;

	IColorFrame* pColorFrame = NULL;
	IDepthFrame* pDepthFrame = NULL;
	IBodyIndexFrame* pBodyIndexFrame = NULL;
	IBodyFrame* pBodyFrame = NULL;


	IMultiSourceFrame* pFrame = NULL;

	HRESULT hr = m_pFrameReader->AcquireLatestFrame(&pFrame);

	//
	// Depth Frame data
	//
	if (SUCCEEDED(hr)){
		IDepthFrameReference* frameRef = 0;
		hr = pFrame->get_DepthFrameReference(&frameRef);
		if (SUCCEEDED(hr)) {
			hr = frameRef->AcquireFrame(&pDepthFrame);
		}
		SafeRelease(frameRef);
	}
	if (SUCCEEDED(hr)) {
		IFrameDescription* pDepthFrameDescription = NULL;
		int depthWidth = 0;
		int depthHeight = 0;
		USHORT nDepthMinReliableDistance = 0;
		USHORT nDepthMaxDistance = 0;
		UINT nDepthBufferSize = 0;
		UINT16 *pDepthBuffer = NULL;
		if (SUCCEEDED(hr)) {
			hr = pDepthFrame->get_RelativeTime(&timeStampDepth);
		}
		if (SUCCEEDED(hr)){
			hr = pDepthFrame->get_FrameDescription(&pDepthFrameDescription);
		}
		if (SUCCEEDED(hr)){
			hr = pDepthFrameDescription->get_Width(&depthWidth);
		}
		if (SUCCEEDED(hr)){
			hr = pDepthFrameDescription->get_Height(&depthHeight);
		}
		if (SUCCEEDED(hr)){
			hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
		}
		if (SUCCEEDED(hr)){
			// In order to see the full range of depth (including the less reliable far field depth)
			// we are setting nDepthMaxDistance to the extreme potential depth threshold
			nDepthMaxDistance = USHRT_MAX;

			// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
			//// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
		}
		if (SUCCEEDED(hr)){
			hr = pDepthFrame->AccessUnderlyingBuffer(&nDepthBufferSize, &pDepthBuffer);
		}
		if (SUCCEEDED(hr)){
			memcpy(m_pDepth, pDepthBuffer, nDepthBufferSize*sizeof(UINT16));
		}
		SafeRelease(pDepthFrameDescription);
	}
	SafeRelease(pDepthFrame);


	//
	// Color Frame Data
	//
	if (SUCCEEDED(hr)){
		IColorFrameReference* frameRef = 0;
		hr = pFrame->get_ColorFrameReference(&frameRef);
		if (SUCCEEDED(hr)) {
			hr = frameRef->AcquireFrame(&pColorFrame);
		}
		SafeRelease(frameRef);
	}

	if (SUCCEEDED(hr)) {
		IFrameDescription* pColorFrameDescription = NULL;
		int colorWidth = 0;
		int colorHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nColorBufferSize = 0;
		RGBQUAD *pColorBuffer = NULL;
		if (SUCCEEDED(hr)) {
			hr = pColorFrame->get_RelativeTime(&timeStampColor);
		}
		if (SUCCEEDED(hr)){
			hr = pColorFrame->get_FrameDescription(&pColorFrameDescription);
		}
		if (SUCCEEDED(hr)){
			hr = pColorFrameDescription->get_Width(&colorWidth);
		}
		if (SUCCEEDED(hr)){
			hr = pColorFrameDescription->get_Height(&colorHeight);
		}
		if (SUCCEEDED(hr)){
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}
		if (SUCCEEDED(hr)){
			if (imageFormat == ColorImageFormat_Bgra){
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nColorBufferSize, reinterpret_cast<BYTE**>(&pColorBuffer));
			}
			else if (m_pColorRGBX)
			{
				pColorBuffer = m_pColorRGBX;
				nColorBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize, reinterpret_cast<BYTE*>(pColorBuffer), ColorImageFormat_Bgra);
			}
			else
			{
				hr = E_FAIL;
			}
		}
		SafeRelease(pColorFrameDescription);
	}
	SafeRelease(pColorFrame);


	

	//
	// BodyIndex Frame data
	//
	if (SUCCEEDED(hr)){
		IBodyIndexFrameReference* frameRef = 0;
		hr = pFrame->get_BodyIndexFrameReference(&frameRef);
		if (SUCCEEDED(hr)) {
			hr = frameRef->AcquireFrame(&pBodyIndexFrame);
		}
		SafeRelease(frameRef);
	}
	if (SUCCEEDED(hr)) {
		IFrameDescription* pBodyIndexFrameDescription = NULL;
		int bodyIndexWidth = 0;
		int bodyIndexHeight = 0;
		UINT nBodyIndexBufferSize = 0;
		UINT8 *pBodyIndexBuffer = NULL;

		if (SUCCEEDED(hr)) {
			hr = pBodyIndexFrame->get_RelativeTime(&timeStampBodyIndex);
		}
		if (SUCCEEDED(hr)) {
			hr = pBodyIndexFrame->get_FrameDescription(&pBodyIndexFrameDescription);
		}
		if (SUCCEEDED(hr)) {
			hr = pBodyIndexFrameDescription->get_Width(&bodyIndexWidth);
		}
		if (SUCCEEDED(hr)) {
			hr = pBodyIndexFrameDescription->get_Height(&bodyIndexHeight);
		}
		if (SUCCEEDED(hr)) {
			hr = pBodyIndexFrame->AccessUnderlyingBuffer(&nBodyIndexBufferSize, &pBodyIndexBuffer);
		}
		if (SUCCEEDED(hr)) {
			memcpy(m_pBodyIndex, pBodyIndexBuffer, bodyIndexWidth * bodyIndexHeight * sizeof(UINT8));
		}
		SafeRelease(pBodyIndexFrameDescription);
	}
	SafeRelease(pBodyIndexFrame);

	//
	// Body Frame data
	//
	if (SUCCEEDED(hr)){
		IBodyFrameReference* frameRef = 0;
		hr = pFrame->get_BodyFrameReference(&frameRef);
		if (SUCCEEDED(hr)) {
			hr = frameRef->AcquireFrame(&pBodyFrame);
		}
		SafeRelease(frameRef);
	}
	if (SUCCEEDED(hr)) {
		IBody* kinectBodies[BODY_COUNT] = { NULL };
		//UINT nBodyIndexBufferSize = 0;
		//UINT8 *pBodyIndexBuffer = NULL;

		if (SUCCEEDED(hr)) {
			hr = pBodyFrame->get_RelativeTime(&timeStampBody);
		}
		
		if (SUCCEEDED(hr)) {
			hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, kinectBodies);
		}
		if (SUCCEEDED(hr)) {
			m_Skeleton = cv::Mat::zeros(BODY_COUNT, 26, CV_32FC4);
			for (UINT8 i = 0; i < BODY_COUNT; ++i) {
				IBody* kinectBody = kinectBodies[i];
				if (kinectBody != 0) {
					UINT8 isTracked = false;
					hr = kinectBody->get_IsTracked(&isTracked);
					if (SUCCEEDED(hr) && isTracked) {
						Joint joints[JointType_Count];
						kinectBody->GetJoints(JointType_Count, joints);

						JointOrientation jointOrientations[JointType_Count];
						kinectBody->GetJointOrientations(JointType_Count, jointOrientations);

						UINT64 id = 0;
						kinectBody->get_TrackingId(&id);

						//std::map<JointType, Body::Joint> jointMap;
						for (UINT32 j = 0; j < JointType_Count; ++j) {
							cv::Vec4f point;
							point[0] = joints[j].Position.X;
							point[1] = joints[j].Position.Y;
							point[2] = joints[j].Position.Z;
							point[3] = joints[j].TrackingState; // 0,1,2;
							m_Skeleton.at<cv::Vec4f>(i, j) = point;
						}

						// Set TrackingID to Detect Face
						UINT64 trackingId = _UI64_MAX;
						hr = kinectBodies[i]->get_TrackingId(&trackingId);
						if (SUCCEEDED(hr)){
							m_pFaceSource[i]->put_TrackingId(trackingId);
						}
					}
				}
			}
		}

		
	}
	SafeRelease(pBodyFrame);

	//
	//  Face Frame data
	//

	for (int count = 0; count < BODY_COUNT; count++){
		IFaceFrame* pFaceFrame = nullptr;
		hr = m_pFaceReader[count]->AcquireLatestFrame(&pFaceFrame);
		if (SUCCEEDED(hr) && pFaceFrame != nullptr){
			IFaceFrameResult* pFaceResult = nullptr;
			hr = pFaceFrame->get_FaceFrameResult(&pFaceResult);
			if (SUCCEEDED(hr) && pFaceResult != nullptr){
//				std::vector<std::string> result;
				std::cout << "FACE " << std::endl;
				// Face Point
				PointF facePoint[FacePointType::FacePointType_Count];
				hr = pFaceResult->GetFacePointsInColorSpace(FacePointType::FacePointType_Count, facePoint);
				if (SUCCEEDED(hr)){
					m_Faces.at<unsigned short>(count, 5) = facePoint[0].X;
					m_Faces.at<unsigned short>(count, 6) = facePoint[0].Y;
					m_Faces.at<unsigned short>(count, 7) = facePoint[1].X;
					m_Faces.at<unsigned short>(count, 8) = facePoint[1].Y;
					// facePoint[0] - Eye (Left)
					// facePoint[1] - Eye (Right)
					// facePoint[2] - Nose
					// facePoint[3] - Mouth (Left)
					// facePoint[4] - Mouth (Right)
				}

				// Face Bounding Box
				RectI boundingBox;
				hr = pFaceResult->get_FaceBoundingBoxInColorSpace(&boundingBox);
				if (SUCCEEDED(hr)){
					m_Faces.at<unsigned short>(count, 0) = boundingBox.Left;
					m_Faces.at<unsigned short>(count, 1) = boundingBox.Top;
					m_Faces.at<unsigned short>(count, 2) = boundingBox.Right - boundingBox.Left;
					m_Faces.at<unsigned short>(count, 3) = boundingBox.Bottom - boundingBox.Top;
				}

				UINT64 trackingId;
				hr = pFaceResult->get_TrackingId(&trackingId);
				if (SUCCEEDED(hr)){
					m_Faces.at<unsigned short>(count, 4) = trackingId;
				}


				// Face Rotation
				Vector4 faceRotation;
				hr = pFaceResult->get_FaceRotationQuaternion(&faceRotation);
				if (SUCCEEDED(hr)){
					int pitch, yaw, roll;
					ExtractFaceRotationInDegrees(&faceRotation, &pitch, &yaw, &roll);
					std::cout << ("Pitch, Yaw, Roll : " + std::to_string(pitch) + ", " + std::to_string(yaw) + ", " + std::to_string(roll)) << std::endl;
				}

				// Face Property
				DetectionResult faceProperty[FaceProperty::FaceProperty_Count];
				hr = pFaceResult->GetFaceProperties(FaceProperty::FaceProperty_Count, faceProperty);
				if (SUCCEEDED(hr)){
					for (int count = 0; count < FaceProperty::FaceProperty_Count; count++){
						switch (faceProperty[count]){
						case DetectionResult::DetectionResult_Unknown:
							std::cout << count << " : Unknown" << std::endl;
							break;
						case DetectionResult::DetectionResult_Yes:
							std::cout << count << " : Yes" << std::endl;
							break;
						case DetectionResult::DetectionResult_No:
							std::cout << count << " : No" << std::endl;
							break;
						case DetectionResult::DetectionResult_Maybe:
							std::cout << count << " : Mayby" << std::endl;
							break;
						default:
							break;
						}
					}
				}
			}
			SafeRelease(pFaceResult);
		}
		SafeRelease(pFaceFrame);
	}

	SafeRelease(pFrame);
}

cv::Mat KinectSensor::GetColorImage() {
	return cv::Mat(cColorHeight, cColorWidth, CV_8UC4, reinterpret_cast<void*>(m_pColorRGBX));
}

cv::Mat KinectSensor::GetDepthImage() {
	return cv::Mat(cDepthHeight, cDepthWidth, CV_16UC1, reinterpret_cast<void*>(m_pDepth));
}

cv::Mat KinectSensor::GetBodyIndexImage() {
	return cv::Mat(cBodyIndexHeight, cBodyIndexWidth, CV_8UC1, reinterpret_cast<void*>(m_pBodyIndex));
}

cv::Mat KinectSensor::GetSkeletons(){
	return m_Skeleton;
}

cv::Mat KinectSensor::GetFaces(){
	return m_Faces;
}