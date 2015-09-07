#include <iostream>
#include <set>
#include "biosandbox/Modules.h"


POSTPROCESSING_MODULE( SaveDatabaseToFile )

using namespace cv;

template<class T>
    std::string toString(const T& t)
{
     std::ostringstream stream;
     stream << t;
     return stream.str();
}

bool SaveDatabaseToFile::Process(vector<Sample *> & rSamplesToProcess){
	Mat personNumTruthMat(1,rSamplesToProcess.size(),CV_32SC1);
	if(rSamplesToProcess.size() <=0){
		cerr<< "ERROR: SaveDatabaseToFile: No sample to train!!"<<endl;
		return false;
	}
	set<std::string> strAttributes;
	int i = 0;
	for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
			it!=rSamplesToProcess.end();
			it++){
		personNumTruthMat.at<int>(i++) = (*it)->GetIdentity();

		// create map of atributes
		map<string, cv::Mat> * attr_map = (*it)->GetMatricesMap();
		for(map<string, cv::Mat>::iterator a_it = attr_map->begin();
			a_it != attr_map->end();
			a_it++){
			if (strAttributes.find(a_it->first) == strAttributes.end()){
				strAttributes.insert(a_it->first);	
			}
		}
	}

	if(mAttributes.find("database") == mAttributes.end()){
		cerr<<"ERROR: SaveDatabaseToFile: set attribute 'database' "<<endl;
		return false;
	}

	FileStorage storage(mAttributes["database"] ,FileStorage::WRITE);

	storage << "trainPersonNumMat" << personNumTruthMat;

	storage << "numberOfAttributes" << (int)(strAttributes.size());
	i=0;
	for(set<string>::iterator s_it = strAttributes.begin();
		s_it != strAttributes.end();
		s_it++){
		string attrName = "attribute_" + toString<int>(i++);
		storage << attrName << *s_it;
	}

	cout<< "Saving trainin samples to file " << mAttributes["database"] << " ..." << endl;
	i = 0;
	for(	vector<Sample *>::iterator it = rSamplesToProcess.begin();
			it!=rSamplesToProcess.end();
			it++){
		for(set<string>::iterator s_it = strAttributes.begin();
			s_it != strAttributes.end();
			s_it++){
			string matNameWithId = *s_it + "_" + toString<int>( i);
			storage << matNameWithId << (*it)->GetMatrix(*s_it); 	
		}
		i++;
	}
	cout << i << " samples saved." <<endl;

	storage.release();
	return true;
}