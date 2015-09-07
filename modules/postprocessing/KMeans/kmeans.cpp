#include <iostream>
#include "biosandbox/Modules.h"

#ifdef WIN32 
	#include <windows.h>
#endif
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/ml/ml.hpp>


class KMeans : public PostprocessingModule {
public:
	KMeans() : kMeansCenters(4), id(0), initialized(false), clusteringData("") {};
	~KMeans(){};
	bool Process(vector<Sample *> & rSamplesToProcess);

private:
	int kMeansCenters;
	int id;
	bool initialized;
	string path;
	string clusteringData;
};
INTERN_LIBRARY_INTERFACE( KMeans )


using namespace cv;




double euclideanDistance(Mat first, Mat second){
	Mat squaredDiff(first.size(),first.type());
	subtract(first,second,squaredDiff);
	pow(squaredDiff,2, squaredDiff);

	//   there is no need to compute SQRT() of squaredDiff, 
	//   classification results would be the same ...
	//   in case you want to be propper uncoment next line:
	
	// return sqrt(sum(squaredDiff)[0]);

	return sum(squaredDiff)[0];
}

int findNearest(Mat samples, Mat checkingSample){
	int index = 0;
	double leastDistSq = DBL_MAX;
	for( int i = 0;	i < samples.rows; i++){
		double ed = euclideanDistance(samples.row(i),checkingSample);
		if(ed < leastDistSq){
			leastDistSq = ed;
			index = i;
		}
	}
	return index;
}


void ShowMat(const char * name, Mat mat, bool f){
	double _min;
	double _max;
	
	Mat m = mat.clone();
	
	cv::minMaxLoc(m,&_min,&_max);
	m = m - _min;
	if(f){
		m = m/(_max - _min);
	} else{
		m = m*255/(_max - _min);
	}
	imshow( name,  m);
}


bool KMeans::Process(vector<Sample *> & rSamplesToProcess){
	vector<Mat> db;
	
	if(!initialized){
		if(mAttributes.find("kMeansCenters") != mAttributes.end()){
			istringstream convert(mAttributes["kMeansCenters"]);
			convert >> kMeansCenters;
		}

		if(mAttributes.find("attribute") != mAttributes.end()){
			clusteringData = mAttributes["attribute"];
		}

		if(mAttributes.find("id") != mAttributes.end()){
			istringstream convert(mAttributes["id"]);
			convert >> id;
		}
		if(mAttributes.find("path") != mAttributes.end()){
			path = mAttributes["path"];
		}
		
		initialized = true;
	}


	for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
			it != rSamplesToProcess.end();
			it++){
				if(clusteringData != "")
					db.push_back((*it)->GetMatrix(clusteringData));
				else
					db.push_back((*it)->GetImage());
	}

	int total = db[0].rows * db[0].cols;
   // build matrix (rows)
    Mat mat( db.size(),total, CV_32FC1);
    for(int i = 0; i < db.size(); i++) {
        Mat X = mat.row(i);
        db[i].reshape( 1,1).row(0).convertTo(X, CV_32FC1, 1);
    }

	 cout<<"rows MAT: \n"<< mat.rows <<endl;
	 cout<<"cols MAT: \n"<< mat.cols <<endl;
	 
	// Run the KMEANS algorithm 
	Mat labellist(db.size(),1, CV_16UC1);
	Mat centers(kMeansCenters, 1, CV_32FC1);//CV_8UC1);
	TermCriteria termcrit(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 10, 1.0);
	kmeans(mat, kMeansCenters, labellist, termcrit, 3, cv::KMEANS_PP_CENTERS, centers);
	
	//cout<<"rows Centers: \n"<< centers.rows <<endl;
	//cout<<"cols Centers: \n"<< centers.cols <<endl;
	//cout<< "labellist Centers: \n"<<labellist <<endl;

	for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
			it != rSamplesToProcess.end();
			it++){
			delete *it;
	}
	rSamplesToProcess.clear();

	for(int i = 0; i < centers.rows; i++) {
        Mat image;
		Mat neighbor;
//		int nearest = findNearest(mat, centers.row(i));
//		char imageName[MAX_PATH];
		//int count = 0;
		//for(int i = 0; i < labellist.rows;i++){
		//	if(labellist.at<int>(i,0) == i) count ++;
		//}

		//cout << "Image index: " << nearest  << " {" << count  << "}"<< endl;

		centers.row(i).reshape(1, db[0].rows).convertTo(image, CV_32FC1, 1);
		Sample * s = new Sample(Mat(),id);
		s->AddMatrix("features",image);
		rSamplesToProcess.push_back(s);

//		sprintf(imageName, "%05d0%05d.png",id,i);
//		imwrite(path + imageName,image);
		//mat.row(nearest).reshape(1, db[0].rows).convertTo(neighbor, CV_8UC1, 1);
		//ShowMat( "Image", image,false );                  
		//ShowMat( "Neighbor", neighbor,false );      
		//ShowMat( "vector", rSamplesToProcess[nearest]->GetImage(),false );      
 		//waitKey(0);
    }
	return true;
}