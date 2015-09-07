#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <set>
#include "biosandbox/Modules.h"

POSTPROCESSING_MODULE( ChiSquareDistanceMatching )

using namespace cv;

bool initialized = false;

bool writePajek = false;
std::string pajekFile = "";

bool filterTag = false;
std::string filteringTag = "";


vector<Mat> gTrainingSamples;
vector<int> gTruth;
map<int,int> identitiesSet;
vector<int> identities;
Mat idTemplate;
double gThreshold = 0;
bool showDistances = true;
//Mat gTruth;


template<class T>
    std::string toString(const T& t)
{
     std::ostringstream stream;
     stream << t;
     return stream.str();
}

double chiSquareDistance(Mat first, Mat second){
	Mat difference;
	subtract(first,second,difference);
	pow(difference,2, difference);

	Mat sums;
	add(first,second,sums);

	divide(difference,sums,difference,0.5);
	return sum(difference)[0];
}

Mat findNearest(vector<Mat> trainingSamples, Mat checkingSample, int * index){
	double leastDistSq = DBL_MAX;
	Mat differences = idTemplate.clone();
	int i=0; 
	for(	vector<Mat>::iterator it = trainingSamples.begin();
			it!=trainingSamples.end();
			it++){
		double ed = chiSquareDistance(*it,checkingSample);


		int pos = identitiesSet[gTruth[i]];

		if(differences.at<float>(0,pos) > ed){
			differences.at<float>(0,pos) = ed;
		}

		if(ed < leastDistSq){
			leastDistSq = ed;
			*index = i;
		}
		i++;
	}
	return differences;
}



pair< vector<int>, vector<Mat> > loadTrainedPerson(string fullPathToFile){
	Mat truth;
	vector<Mat> trainingSamples;
	FileStorage storage(fullPathToFile ,FileStorage::READ);

	if(!storage.isOpened()){
		cerr<<"ERROR: ChiSquareDistanceMatching: Unable to open "<< fullPathToFile <<endl;
		return pair<Mat,vector<Mat>>(truth,trainingSamples);
	}
	storage["trainPersonNumMat"]>>truth;
	for(int i = 0; i< truth.cols; i++){
		Mat features;
		string featuresName = "features_" + toString<int>( i );
		storage[featuresName]>>features;
		trainingSamples.push_back(features);
		gTrainingSamples.push_back(features);
		gTruth.push_back(truth.at<int>(0,i));
		if(identitiesSet.find(truth.at<int>(0,i)) == identitiesSet.end() ) {
			identities.push_back(truth.at<int>(0,i));
			identitiesSet[ truth.at<int>(0,i) ] = identitiesSet.size();
		}
	}
	storage.release();
	return pair< vector<int>, vector<Mat>>((vector<int>)truth,trainingSamples);
}

pair< vector<int>, vector<Mat> > checkAndLoadDatabaseFile(string path, string dirName){
	string pattern = path + "\\" + dirName + "\\*.bio.db";
	struct _finddata_t bioDbFile;
	long hFile;	
	pair< vector<int>, vector<Mat> > retPair;
	hFile = _findfirst(pattern.c_str() , &bioDbFile );
	for(int ret = 0; 
		hFile != -1L && ret == 0;
		ret = _findnext( hFile, &bioDbFile )){
		loadTrainedPerson(path + "\\" + dirName + "\\" + bioDbFile.name);
	}
	if(hFile != -1L ) _findclose( hFile );
	return retPair;
}

 pair< vector<int>, vector<Mat> > loadMultipleDatabaseFiles(string path){
	string searchPath = path + "\\*";
	struct _finddata_t bioDbFile;
	pair< vector<int>, vector<Mat> > retPair;
	long hFile;
	hFile = _findfirst(searchPath.c_str() , &bioDbFile );
	for(int ret = 0; 
		hFile != -1L && ret == 0;
		ret = _findnext( hFile, &bioDbFile )){
			string name = bioDbFile.name;
			if(name == "." || name == "..") continue;
			if(bioDbFile.attrib & FILE_ATTRIBUTE_DIRECTORY){
				checkAndLoadDatabaseFile(path,name);
			}
	}
	if(hFile != -1L ) _findclose( hFile );
	return retPair;
	/*if( (hFile = _findfirst(path.c_str() , &bioDbFile )) == -1L ){
       cout << "No modules available!" << endl;
	} else {
		cout << "List of all available modules:" << endl;
		do {
			BaseModule * module;
			if((module = dl->LoadModuleFrom(dllFile.name)) != 0) {
				allModules.push_back(module);
				cout << "\t" << dllFile.name << " ("<< getModuleTypeName(module->getType()) <<")"<< endl;
			}
		} while( _findnext( hFile, &dllFile ) == 0 );
		_findclose( hFile );
   }*/
}



bool ChiSquareDistanceMatching::Process(vector<Sample *> & rSamplesToProcess){
	if(!initialized){
		if(mAttributes.find("filterTag") != mAttributes.end()){
			filterTag = true;
			filteringTag = mAttributes["filterTag"];
		}

		if(mAttributes.find("threshold") != mAttributes.end()){
			gThreshold = atof(mAttributes["threshold"].c_str());
		}

		if(mAttributes.find("database") == mAttributes.end()){
			cerr<<"ERROR: ChiSquareDistanceMatching: set attribute 'database' "<<endl;
			return false;
		}
		if (mAttributes.find("exportPajek") != mAttributes.end()){
			writePajek = true;
			pajekFile = mAttributes["exportPajek"];
		}
		
		loadMultipleDatabaseFiles(mAttributes["database"]);
		initialized = true;

		idTemplate =  Mat(1,identities.size(),CV_32FC1);
		for(int i = 0; i < identities.size(); i++) idTemplate.at<float>(0,i) = FLT_MAX;
	}

	
	int rightClassified = 0;
	int truePositive = 0;
	int trueNegative = 0;
	int falsePositive = 0;
	int falseNegative = 0;
	for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
			it != rSamplesToProcess.end();
			it++){
		if(filterTag && !((*it)->HasMatrix(filteringTag))){
			continue;
		}

		int nearest = 0;
		Mat differences = findNearest(gTrainingSamples,(*it)->GetMatrix("features"),&nearest);
		if (showDistances){
			if((*it)->GetIdentity() == gTruth.at(nearest)){
				rightClassified++;
			}
			if(gThreshold > 0) {
				for(int i = 0; i < differences.cols;i++){
					int id = (*it)->GetIdentity();
					if(id == identities[i]){
						if(gThreshold >= differences.at<float>(0,i)){
							truePositive++;
						} else {
							falseNegative++;			
						}
					} else{
						if(gThreshold < differences.at<float>(0,i)){
							trueNegative++;
						} else{
							falsePositive++;
						}
					}
					//	cout << differences.at<float>(0,i) << "("<< identities[i]<<","<<nearest << ")"<< " \t" ;
				}
		
			}
			cout<< (*it)->GetIdentity()<< " " << gTruth.at(nearest) << " ";
		}
		(*it)->AddMatrix("differences",differences);
		(*it)->AddMatrix("identities", (Mat)identities);
		
		if (showDistances) {
			for(int i = 0; i < differences.cols;i++){
				cout << identities[i] << " " << differences.at<float>(0,i) << " ";
			}
			cout << endl;
		}
	}
	if (showDistances) {
		cout << "Precision: " << ((double)rightClassified)/((double)rSamplesToProcess.size())<< "  "<< "  "<<"  TP:" << truePositive<<"  TN:" << trueNegative<<"  FP:" << falsePositive<<"  FN:" << falseNegative  << endl;
	}
	return true;
}