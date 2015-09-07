//------------------------------------------------------------------------------
// <copyright file="FTHelper2.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include "FTHelper2.h"


using namespace std;
using namespace cv;

#ifdef SAMPLE_OPTIONS
#include "Options.h"
#else
PVOID _opt = NULL;
#endif

FTHelper2::FTHelper2()
{
    m_UserContext = 0;
    m_colorImage = NULL;
    m_depthImage = NULL;
    m_ApplicationIsRunning = false;
    m_CallBack = NULL;
    m_CallBackParam = NULL;
    m_UserSelectCallBack = NULL;
    m_UserSelectCallBackParam = NULL;
    m_XCenterFace = 0;
    m_YCenterFace = 0;
    m_hFaceTrackingThread = NULL;
    m_DrawMask = TRUE;
    m_depthType = NUI_IMAGE_TYPE_DEPTH;
    m_depthRes = NUI_IMAGE_RESOLUTION_INVALID;
    m_bNearMode = FALSE;
    m_colorType = NUI_IMAGE_TYPE_COLOR;
    m_colorRes = NUI_IMAGE_RESOLUTION_INVALID;
	m_bSeatedSkeleton = FALSE;
}

FTHelper2::~FTHelper2()
{
    Stop();
}

HRESULT FTHelper2::Init(UINT nbUsers, FTHelper2CallBack callBack, PVOID callBackParam, FTHelper2UserSelectCallBack userSelectCallBack, PVOID userSelectCallBackParam,
    NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, BOOL bSeatedSkeleton)
{
    //if (!callBack)
    //{
    //    return E_INVALIDARG;
    //}
    m_CallBack = callBack;
    m_CallBackParam = callBackParam;
    m_UserSelectCallBack = userSelectCallBack;
    m_UserSelectCallBackParam = userSelectCallBackParam;
    m_nbUsers = nbUsers;
    m_ApplicationIsRunning = true;
    m_depthType = depthType;
    m_depthRes = depthRes;
    m_bNearMode = bNearMode;
	m_bSeatedSkeleton = bSeatedSkeleton;
    m_colorType = colorType;
    m_colorRes = colorRes;
    m_hFaceTrackingThread = CreateThread(NULL, 0, FaceTrackingStaticThread, (PVOID)this, 0, 0);
	InitializeCriticalSection(&critSec);
    return S_OK;
}

HRESULT FTHelper2::Stop()
{
    m_ApplicationIsRunning = false;
    if (m_hFaceTrackingThread)
    {
        WaitForSingleObject(m_hFaceTrackingThread, 1000);
    }
    m_hFaceTrackingThread = 0;

    if (m_UserContext != 0)
    {
        for (UINT i=0; i<m_nbUsers; i++)
        {
            if (m_UserContext[i].m_pFTResult != 0)
            {
                m_UserContext[i].m_pFTResult->Release();
                m_UserContext[i].m_pFTResult = 0;
            }
            if (m_UserContext[i].m_pFaceTracker != 0)
            {
                m_UserContext[i].m_pFaceTracker->Release();
                m_UserContext[i].m_pFaceTracker = 0;
            }
        }
        delete[] m_UserContext;
        m_UserContext = 0;
    }

    if (m_colorImage != NULL)
    {
        m_colorImage->Release();
        m_colorImage = NULL;
    }

    if (m_depthImage != NULL)
    {
        m_depthImage->Release();
        m_depthImage = NULL;
    }

    m_CallBack = NULL;
    return S_OK;
}

DWORD s_ColorCode[] = {0x00FFFF00, 0x00FF0000,  0x0000FF00, 0x0000FFFF, 0x00FF00FF, 0x000000FF};


BOOL FTHelper2::SubmitFraceTrackingResult(IFTResult* pResult, UINT userId)
{
    if (pResult != NULL && SUCCEEDED(pResult->GetStatus()))
    {
        if (m_CallBack)
        {
            (*m_CallBack)(m_CallBackParam, userId);
        }

        if (m_DrawMask)
        {
            FLOAT* pSU = NULL;
            UINT numSU;
            BOOL suConverged;
            m_UserContext[userId].m_pFaceTracker->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);
            POINT viewOffset = {0, 0};
            FT_CAMERA_CONFIG cameraConfig;
            if (m_KinectSensorPresent)
            {
                m_KinectSensor.GetVideoConfiguration(&cameraConfig);
            }
            else
            {
                cameraConfig.Width = 640;
                cameraConfig.Height = 480;
                cameraConfig.FocalLength = 500.0f;
            }
            IFTModel* ftModel;
            HRESULT hr = m_UserContext[userId].m_pFaceTracker->GetFaceModel(&ftModel);
            if (SUCCEEDED(hr))
            {
                DWORD color = s_ColorCode[userId%6];
                //hr = VisualizeFaceModel(m_colorImage, ftModel, &cameraConfig, pSU, 1.0, viewOffset, pResult, color);
                ftModel->Release();
            }
        }
    }
    return TRUE;
}

// We compute here the nominal "center of attention" that is used when zooming the presented image.
void FTHelper2::SetCenterOfImage(IFTResult* pResult)
{
    float centerX = ((float)m_colorImage->GetWidth())/2.0f;
    float centerY = ((float)m_colorImage->GetHeight())/2.0f;
    if (pResult)
    {
        if (SUCCEEDED(pResult->GetStatus()))
        {
            RECT faceRect;
            pResult->GetFaceRect(&faceRect);
            centerX = (faceRect.left+faceRect.right)/2.0f;
            centerY = (faceRect.top+faceRect.bottom)/2.0f;
        }
        m_XCenterFace += 0.02f*(centerX-m_XCenterFace);
        m_YCenterFace += 0.02f*(centerY-m_YCenterFace);
    }
    else
    {
        m_XCenterFace = centerX;
        m_YCenterFace = centerY;
    }
}

// Get a video image and process it.
// We employ special code to associate a user ID with a tracker.

void FTHelper2::CheckCameraInput()
{
    HRESULT hrFT = E_FAIL;

	Mat ids(1,6,CV_16UC1);

    if (m_KinectSensorPresent && m_KinectSensor.GetVideoBuffer())
    {
        HRESULT hrCopy = m_KinectSensor.GetVideoBuffer()->CopyTo(m_colorImage, NULL, 0, 0);

		if (SUCCEEDED(hrCopy) && m_KinectSensor.GetDepthBuffer())
        {
			Mat m(m_colorImage->GetHeight(),m_colorImage->GetWidth(),
				CV_8UC4,(void *)(m_colorImage->GetBuffer()));
			EnterCriticalSection(&critSec);
			currentColorImage = m;
			LeaveCriticalSection(&critSec);

            hrCopy = m_KinectSensor.GetDepthBuffer()->CopyTo(m_depthImage, NULL, 0, 0);
			if (SUCCEEDED(hrCopy))
			{
				Mat m(m_depthImage->GetHeight(),m_depthImage->GetWidth(),
					CV_16UC1,(void *)(m_depthImage->GetBuffer()));
				EnterCriticalSection(&critSec);
				currentDepthImage = m;
				LeaveCriticalSection(&critSec);
			}
        }
		// Copy skeletons
		m_skeletons = m_KinectSensor.Skeletons();

        // Do face tracking
		
        if (SUCCEEDED(hrCopy))
        {
			int count = 0;
            FT_SENSOR_DATA sensorData(m_colorImage, m_depthImage, m_KinectSensor.GetZoomFactor(), m_KinectSensor.GetViewOffSet());

            if (m_UserSelectCallBack != NULL)
            {
                (*m_UserSelectCallBack)(m_UserSelectCallBackParam, &m_KinectSensor, m_nbUsers, m_UserContext);
            }
            else
            {
                SelectUserToTrack(&m_KinectSensor, m_nbUsers, m_UserContext);
            }
            for (UINT i=0; i<m_nbUsers; i++)
            {
			    if (m_UserContext[i].m_CountUntilFailure == 0 ||
                    !m_KinectSensor.IsTracked(m_UserContext[i].m_SkeletonId))
                {
                    m_UserContext[i].m_LastTrackSucceeded = false;
					ids.at<unsigned short>(0,i) = 0;
                    continue;
                }
				ids.at<unsigned short>(0,i) = m_UserContext[i].m_SkeletonId+1;
//				cout << m_UserContext[i].m_SkeletonId+1 << " ";
				count++;
                FT_VECTOR3D hint[2];
                hint[0] =  m_KinectSensor.NeckPoint(m_UserContext[i].m_SkeletonId);
                hint[1] =  m_KinectSensor.HeadPoint(m_UserContext[i].m_SkeletonId);

/**/                if (m_UserContext[i].m_LastTrackSucceeded)
/**/                {
/**/                    hrFT = m_UserContext[i].m_pFaceTracker->ContinueTracking(&sensorData, hint, m_UserContext[i].m_pFTResult);
/**/                }
/**/                else
/**/                {
                    hrFT = m_UserContext[i].m_pFaceTracker->StartTracking(&sensorData, NULL, hint, m_UserContext[i].m_pFTResult);
/**/                }
                m_UserContext[i].m_LastTrackSucceeded = SUCCEEDED(hrFT) && SUCCEEDED(m_UserContext[i].m_pFTResult->GetStatus());
                if (m_UserContext[i].m_LastTrackSucceeded)
                {
                    SubmitFraceTrackingResult(m_UserContext[i].m_pFTResult, i);
                }
                else
                {
                    m_UserContext[i].m_pFTResult->Reset();
                }
			}

			EnterCriticalSection(&critSec);
			m_ids = ids;
			LeaveCriticalSection(&critSec);

//			cout << endl;
			int facenum=0;
			Mat faces(count,9,CV_16SC1);
			vector<float> skeletonIds;
			for (UINT i=0; i<m_nbUsers; i++){
				 if (m_UserContext[i].m_CountUntilFailure == 0 ||
					 !m_KinectSensor.IsTracked(m_UserContext[i].m_SkeletonId)||
					 facenum >= count){
					 continue;
				 }
				FT_VECTOR2D* points;
				UINT pcount;
				HRESULT hr = m_UserContext[i].m_pFTResult->Get2DShapePoints(&points, &pcount);
				if ( FAILED( hr ) ){
					faces.at<unsigned short>(facenum,0) = points[NUI_SKELETON_POSITION_HEAD].x;
					faces.at<unsigned short>(facenum,1) = points[NUI_SKELETON_POSITION_HEAD].y;
					faces.at<unsigned short>(facenum,2) = 0;
					faces.at<unsigned short>(facenum,3) = 0;
					faces.at<unsigned short>(facenum,4) = m_UserContext[i].m_SkeletonId;
				}	else {
					faces.at<unsigned short>(facenum,0) = 0;
					faces.at<unsigned short>(facenum,1) = 0;
					faces.at<unsigned short>(facenum,2) = 0;
					faces.at<unsigned short>(facenum,3) = 0;
					faces.at<unsigned short>(facenum,4) = m_UserContext[i].m_SkeletonId;
				}


				
				if(m_UserContext[i].m_LastTrackSucceeded){
					skeletonIds.push_back(m_UserContext[i].m_SkeletonId);
					RECT rect;
					FT_VECTOR2D * points;
					UINT pointsCount;
					hr = m_UserContext[i].m_pFTResult->Get2DShapePoints(&points,&pointsCount);
					if( SUCCEEDED( hr ) ) {
						faces.at<unsigned short>(facenum,5) = (points[0].x+points[4].x)/2;
						faces.at<unsigned short>(facenum,6) = (points[0].y+points[4].y)/2;
						faces.at<unsigned short>(facenum,7) = (points[8].x+points[12].x)/2;
						faces.at<unsigned short>(facenum,8) = (points[8].y+points[12].y)/2;
					}

					hr = m_UserContext[i].m_pFTResult->GetFaceRect(&rect);
					if ( SUCCEEDED( hr ) )
					{
						faces.at<unsigned short>(facenum,0) = rect.left;
						faces.at<unsigned short>(facenum,1) = rect.top;
						faces.at<unsigned short>(facenum,2) = rect.right - rect.left;
						faces.at<unsigned short>(facenum,3) = rect.bottom - rect.top;
						faces.at<unsigned short>(facenum,4) = m_UserContext[i].m_SkeletonId;
					}
				}
				facenum++;
			}
			EnterCriticalSection(&critSec);
			m_faces = faces;

			LeaveCriticalSection(&critSec);
            //SetCenterOfImage(m_UserContext[i].m_pFTResult);
            
        }
    }
}

DWORD WINAPI FTHelper2::FaceTrackingStaticThread(PVOID lpParam)
{
    FTHelper2* context = static_cast<FTHelper2*>(lpParam);
    if (context)
    {
        return context->FaceTrackingThread();
    }
    return 0;
}

DWORD WINAPI FTHelper2::FaceTrackingThread()
{
    FT_CAMERA_CONFIG videoConfig;
    FT_CAMERA_CONFIG depthConfig;
    FT_CAMERA_CONFIG* pDepthConfig = NULL;

    // Try to get the Kinect camera to work
    HRESULT hr = m_KinectSensor.Init(m_depthType, m_depthRes, m_bNearMode, FALSE, m_colorType, m_colorRes, m_bSeatedSkeleton);
    if (SUCCEEDED(hr))
    {
        m_KinectSensorPresent = TRUE;
        m_KinectSensor.GetVideoConfiguration(&videoConfig);
        m_KinectSensor.GetDepthConfiguration(&depthConfig);
        pDepthConfig = &depthConfig;
    }
    else
    {
        m_KinectSensorPresent = FALSE;

		cerr<< "Could not initialize the Kinect sensor.\n" << "Face Tracker Initialization Error\n" << MB_OK << endl;
        return 1;
    }
    
    m_UserContext = new FTHelperContext[m_nbUsers];
    if (m_UserContext != 0)
    {
        memset(m_UserContext, 0, sizeof(FTHelperContext)*m_nbUsers);
    }
    else
    {
        cerr<< "Could not allocate user context array.\n" << "Face Tracker Initialization Error\n" << MB_OK << endl;
        return 2;
    }

    for (UINT i=0; i<m_nbUsers;i++)
    {
        // Try to start the face tracker.
        m_UserContext[i].m_pFaceTracker = FTCreateFaceTracker(_opt);
        if (!m_UserContext[i].m_pFaceTracker)
        {
            cerr << "Could not create the face tracker.\n" << "Face Tracker Initialization Error\n" << MB_OK << endl;
            return 3;
        }

        hr = m_UserContext[i].m_pFaceTracker->Initialize(&videoConfig, pDepthConfig, NULL, NULL); 
        if (FAILED(hr))
        {
			//WCHAR 
			char path[512], buffer[1024];
            GetCurrentDirectory(ARRAYSIZE(path), path);
            sprintf(buffer, "Could not initialize face tracker (%s) for user %d.\n", path, i);

            cerr << buffer << "Face Tracker Initialization Error\n" << MB_OK << endl;

            return 4;
        }
        m_UserContext[i].m_pFaceTracker->CreateFTResult(&m_UserContext[i].m_pFTResult);
        if (!m_UserContext[i].m_pFTResult)
        {
            cerr << "Could not initialize the face tracker result for user %d.\n" << "Face Tracker Initialization Error\n" << MB_OK << endl;
            return 5;
        }
        m_UserContext[i].m_LastTrackSucceeded = false;
    }

    // Initialize the RGB image.
    m_colorImage = FTCreateImage();
    if (!m_colorImage || FAILED(hr = m_colorImage->Allocate(videoConfig.Width, videoConfig.Height, FTIMAGEFORMAT_UINT8_B8G8R8X8)))
    {
        return 6;
    }
    
    if (pDepthConfig)
    {
        m_depthImage = FTCreateImage();
        if (!m_depthImage || FAILED(hr = m_depthImage->Allocate(depthConfig.Width, depthConfig.Height, FTIMAGEFORMAT_UINT16_D13P3)))
        {
            return 7;
        }
    }

    SetCenterOfImage(NULL);

    while (m_ApplicationIsRunning)
    {
        CheckCameraInput();
        //InvalidateRect(m_hWnd, NULL, FALSE);
        //UpdateWindow(m_hWnd);
        Sleep(16);
    }
    return 0;
}

HRESULT FTHelper2::GetCameraConfig(FT_CAMERA_CONFIG* cameraConfig)
{
    return m_KinectSensorPresent ? m_KinectSensor.GetVideoConfiguration(cameraConfig) : E_FAIL;
}

void FTHelper2::SelectUserToTrack(KinectSensor * pKinectSensor,
    UINT nbUsers, FTHelperContext* pUserContexts)
{
    // Initialize an array of the available skeletons
    bool SkeletonIsAvailable[NUI_SKELETON_COUNT];
    for (UINT i=0; i<NUI_SKELETON_COUNT; i++)
    {
//		cout << "." ;
        SkeletonIsAvailable[i] = pKinectSensor->IsTracked(i);
    }
//	cout << endl;
    // If the user's skeleton is still tracked, mark it unavailable
    // and make sure we will keep associating the user context to that skeleton
    // If the skeleton is not track anaymore, decrease a counter until we 
    // deassociate the user context from that skeleton id.
//    cout << " ----------- ";
	for (UINT i=0; i<nbUsers; i++)
    {
//		cout << " "<<i << " ";

        if (pUserContexts[i].m_CountUntilFailure > 0)
        {
//			cout << "("  <<  pUserContexts[i].m_SkeletonId << ")" ;

            if (SkeletonIsAvailable[pUserContexts[i].m_SkeletonId])
            {
                SkeletonIsAvailable[pUserContexts[i].m_SkeletonId] = false;
                pUserContexts[i].m_CountUntilFailure++;
                if (pUserContexts[i].m_CountUntilFailure > 5)
                {
                    pUserContexts[i].m_CountUntilFailure = 5;
                }
            }
            else
            {
                pUserContexts[i].m_CountUntilFailure--;
            }
        }
    }
//	cout << endl;

    // Try to find an available skeleton for users who do not have one
    for (UINT i=0; i<nbUsers; i++)
    {
        if (pUserContexts[i].m_CountUntilFailure == 0)
        {
            for (UINT j=0; j<NUI_SKELETON_COUNT; j++)
            {
                if (SkeletonIsAvailable[j])
                {
                    pUserContexts[i].m_SkeletonId = j;
                    pUserContexts[i].m_CountUntilFailure = 1;
                    SkeletonIsAvailable[j] = false;
                    break;
                }
            }
        }
    }
}
