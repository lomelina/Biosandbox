#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <fstream>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <sys/timeb.h>

#include "TemporalFilter.h"

#define WIN32_LEAN_AND_MEAN

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#include "biosandbox/Modules.h"

POSTPROCESSING_MODULE(ContiniousTracking)

using namespace cv;

DWORD WINAPI notifyhreads(LPVOID pParam);

bool isTrackerInitialized = false;
CvScalar * colors = new CvScalar[8];
int framesToSkip();

int getMilliSpan(int nTimeStart){
	int nSpan = getMilliCount() - nTimeStart;
	if(nSpan < 0)
		nSpan += 0x100000 * 1000;
	return nSpan;
}

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
		cerr << "Error: TldTracking: unable to open namesFile: " << file <<  endl;
	}
}

bool parsedParams = false;
bool networkNotify = false;
string notifyHost = "127.0.0.1";
string hostPort = "27015";
int timeForSamples = 1300;
double gThreshold = 0;


map<int, TemporalFilter * > gFilters;

bool ContiniousTracking::Process(vector<Sample *> & rSamplesToProcess){
	int num = 1;
	if(!parsedParams){
		if( mAttributes.find("server") != mAttributes.end()) {
			notifyHost = mAttributes["server"];
			networkNotify = true;
		} 
		if( mAttributes.find("port") != mAttributes.end()) {
			hostPort = mAttributes["port"];
		} 

		if( mAttributes.find("timeForSamples") != mAttributes.end()) {
			istringstream ( mAttributes["timeForSamples"] ) >> timeForSamples;
		} 
		parsedParams = true;

		if(mAttributes.find("namesFile") != mAttributes.end()){
			loadNames(mAttributes["namesFile"]);
		}

		if(mAttributes.find("threshold") != mAttributes.end()){
			gThreshold = atof(mAttributes["threshold"].c_str());
		}

		colors[0] = CV_RGB(255,0,0);
		colors[1] = CV_RGB(0,255,0);
		colors[2] = CV_RGB(0,0,255);
		colors[3] = CV_RGB(255,0,255);
		colors[4] = CV_RGB(255,255,0);
		colors[5] = CV_RGB(0,255,255);
		
	}

	Mat frame_grey;
	
	if(!rSamplesToProcess[0]->HasMatrix("GrayImage")){
		cvtColor(rSamplesToProcess[0]->GetImage(), frame_grey, COLOR_BGR2GRAY );
	} else {
		frame_grey = rSamplesToProcess[0]->GetMatrix("GrayImage");
	}

	for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
			it!=rSamplesToProcess.end();
			it++){
		if((*it)->HasMatrix("face")){
			Mat r = (*it)->GetMatrix("face");
			int id = (*it)->GetIdentity();
			TemporalFilter * filter;
			if(gFilters.find(id) == gFilters.end()){
				filter = new TemporalFilter(id,gThreshold);
				gFilters.insert(pair<int, TemporalFilter*>(id,filter));
			} else {
				filter = gFilters[id];
			}
			filter->Update(*it);
			filter->SetFaceRegion(Rect(r.at<unsigned short>(0,0),r.at<unsigned short>(0,1),r.at<unsigned short>(0,2),r.at<unsigned short>(0,3)));
		}
	}

	if(rSamplesToProcess[0]->HasMatrix("faces")){
		Mat regions = rSamplesToProcess[0]->GetMatrix("faces");
		if(regions.cols > 4){
			for(auto it = gFilters.begin(); it!=gFilters.end(); it++){
				Mat skeletons = regions.col(4).clone();
				it->second->NotifyTrackedSkeletons(skeletons);
			}
		}
	}

	vector<TemporalFilter *> toRemove;
	int colorId = 0;
	for(auto it = gFilters.begin();
			it!=gFilters.end();
			it++){
		
			unsigned long notifyTime = getMilliCount() - it->second->GetLastNotification();
			if(notifyTime > 1000 && it->second->GetPersonId() >= 0) {
				it->second->SetLastNotification(getMilliCount());
				CreateThread( NULL, 0, notifyhreads, it->second, 0, NULL );
			}
			
			CvScalar color = colors[min(5,colorId)];
			unsigned long t = getMilliCount();
			if((t - it->second->GetLastHit() > 1000)){
				color = CV_RGB(128,128,128);
			}
			cv::rectangle(rSamplesToProcess[0]->GetImage(), cv::Rect(0,num*20-18,200,20), CV_RGB(0,0,0), CV_FILLED, 0, 0);
			String label = (*it).second->GetPersonId() > -1 ? names[(*it).second->GetPersonId()] : "<unknown>";
			cv::putText(rSamplesToProcess[0]->GetImage(),label,cv::Point(0,num*20),FONT_HERSHEY_COMPLEX_SMALL, 1,colors[min(5,colorId)] , 1);
			

			cv::putText(rSamplesToProcess[0]->GetImage(),label,it->second->GetFaceRegion().tl() + cv::Point(0,-10),FONT_HERSHEY_COMPLEX_SMALL, 1.8, color, 1);
			cv::rectangle(rSamplesToProcess[0]->GetImage(),it->second->GetFaceRegion(), color, 3, 8, 0 );
			
			colorId++;
			
			
			num++;
			
			if((t - it->second->GetLastHit() > 5000)){
				toRemove.push_back(it->second);
			}
	}

	for(auto it = toRemove.begin();
			it != toRemove.end();
			it++){
			TemporalFilter * f = *it;
			gFilters.erase(f->GetTrackingId());
			cout << "Removing " << f->GetTrackingId() << endl;
			delete f;
	}
	toRemove.clear();
	return true;
}

DWORD WINAPI notifyhreads(LPVOID pParam)
{
	cout<< "Sending notification to: "  << notifyHost << endl;
   WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char *sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
	int id = ((TemporalFilter *)pParam)->GetPersonId();
	//int id = ((SampleTimeFilter *)pParam)->GetId();
	//bool present = ((SampleTimeFilter *)pParam)->IsPresent();
	
	string name = "";
	if(names.find(id) != names.end()){
		name = names[id];
	}

	string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><application><name>FACE</name><users><user><name>"+name+"</name><probability>10</probability><presence>" + /*(present ? */"PRESENT" /*: "QUIT")*/+ "</presence></user></users></application>";
	cout << xml << endl << endl;
	//return 0;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
	iResult = getaddrinfo(notifyHost.c_str(), hostPort.c_str(), &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    //iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
	iResult = send( ConnectSocket, xml.c_str(), xml.size(), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();


	return 0;
}