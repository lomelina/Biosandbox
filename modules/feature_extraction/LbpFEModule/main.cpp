#include <iostream>
#include <fstream>
#include <math.h>   
#include "biosandbox/Modules.h"
#include "opencv2/opencv.hpp"


FEATURESEXTRACTION_MODULE(LbpFEModule)




using namespace cv;
using namespace std;

int bitcount(uchar a){
 register unsigned int c=0;
 uchar x = a;
 while(x) 
  {
  c++;
  x &=  x - 1;
  }
 return(c);
}


class Mapping {
public:
	Mapping(){
		
	}
	Mat * Map(Mat * patterns){
		cv::MatIterator_<uchar> it = patterns->begin<uchar>(),
                    it_end = patterns->end<uchar>();
		for(; it != it_end; ++it)
		{
			(*it) = Map((*it));
		}
		return patterns;
	};
	virtual int ResultLength() = 0;
protected:
	virtual int Map(int i) = 0;
};


class U8Mapping: public Mapping {
public:
	U8Mapping(int neighbours): Mapping(){
		int nbCount  = 1<<neighbours;
		mTable = new int[nbCount];
		int index = 0;

		mResultLength = neighbours * (neighbours - 1) + 3;

		for(int i = 0; i< nbCount;i++){
			int j = i<<1 | i>>7;
			int numt = bitcount( i ^ j ); 

			if( numt <= 2){
				mTable[i] = index;
				index = index + 1;
			} else{
				mTable[i] = mResultLength - 1;
			}
		}
	}
	~U8Mapping(){
		delete [] mTable;
	}
	int ResultLength(){
		return mResultLength;
	};
protected:
	int Map(int i) {
		return mTable[i];
	};
private:
	int mResultLength;
	int * mTable;
};

class NoMapping: public Mapping {
public:
	NoMapping(int neighbours): Mapping(){
		length = 1<<8;
	}
	
	~NoMapping(){
		
	}
	int ResultLength(){
		return 256;
	};
protected:
	int Map(int i) {
		return i;
	};
private:
	int length;
};


struct lbpType	{
	Mat spoints;
	int xsize;
	int ysize;
	
	int len;

	double miny;
	double maxy;
	double minx;
	double maxx;
	
	int bsizey;	//ceil(max(maxy,0))-floor(min(miny,0))+1;
	int bsizex;	//ceil(max(maxx,0))-floor(min(minx,0))+1;

	int origy;	//1-floor(min(miny,0));
	int origx;	//1-floor(min(minx,0));
};



struct lbpType * setNeighbors(int radius, int neighbors){
	struct lbpType * lt = new struct lbpType;
	lt->spoints = Mat::zeros(neighbors,2,CV_32FC1);
	float a = (2 * CV_PI)/neighbors;
	for(int i = 0; i< neighbors; i++){
		lt->spoints.at<float>(i,0) = -radius*sin((i)*a);
        lt->spoints.at<float>(i,1) = radius*cos((i)*a);
	}

	lt->len = neighbors;
	//for(int i = 0; i< neighbors; i++){
	//	cout << lt->spoints.at<float>(i,0) << " "  << lt->spoints.at<float>(i,1) << endl;
	//}

	cv::minMaxLoc(lt->spoints.col(0), &(lt->miny),&(lt->maxy));
	cv::minMaxLoc(lt->spoints.col(1), &(lt->minx),&(lt->maxx));

	
	lt->bsizey = ceil(__max(lt->maxy,0))-floor(__min(lt->miny,0))+1;
	lt->bsizex = ceil(__max(lt->maxx,0))-floor(__min(lt->minx,0))+1;

	lt->origy = 1-floor(__min(lt->miny,0));
	lt->origx = 1-floor(__min(lt->minx,0));

	return lt;
}

struct lbpType * setOriginalNeighbors(){
	struct lbpType * lt = new struct lbpType;
	lt->spoints = Mat::zeros(8,2,CV_32FC1);
	float spoints[8][2] = {	{-1,-1},
							{-1,0},
							{-1,1},
							{0,-1},
							{0,1},
							{1,-1},
							{1,0},
							{1,1}};
	lt->len = 8;

	for(int i = 0; i< 8; i++){
		lt->spoints.at<float>(i,0) = spoints[i][0];
        lt->spoints.at<float>(i,1) = spoints[i][1];
	}

	//for(int i = 0; i< 8; i++){
	//	cout << lt->spoints.at<float>(i,0) << " "  << lt->spoints.at<float>(i,1) << endl;
	//}

	cv::minMaxLoc(lt->spoints.col(0), &(lt->miny),&(lt->maxy));
	cv::minMaxLoc(lt->spoints.col(1), &(lt->minx),&(lt->maxx));

	
	lt->bsizey = ceil(__max(lt->maxy,0))-floor(__min(lt->miny,0))+1;
	lt->bsizex = ceil(__max(lt->maxx,0))-floor(__min(lt->minx,0))+1;

	lt->origy = 1-floor(__min(lt->miny,0));
	lt->origx = 1-floor(__min(lt->minx,0));

	return lt;
}

//void saveMatToCsv(char * filename, Mat m){
//	ofstream file;
//	file.open(filename);
//	for (int y = 0; y < m.rows; y++){
//		for (int x = 0; x < m.cols; x++){
//			file << (uint)(m.at<uchar>(x,y)) << ";";
//
//		}
//		file << endl;
//	}
//	file.close();
//}
//
//void saveFloatMatToCsv(char * filename, Mat m){
//	ofstream file;
//	file.open(filename);
//	for (int y = 0; y < m.rows; y++){
//		for (int x = 0; x < m.cols; x++){
//			file << (float)(m.at<float>(y, x)) << ";";
//		}
//		file << endl;
//	}
//	file.close();
//}

Mat lbpmat(Mat img, Mapping * map, struct lbpType * neighbors){
	//int neighbors = 8;
	//float spoints[8][2] = {	{-1,-1},
	//						{-1,0},
	//						{-1,1},
	//						{0,-1},
	//						{0,1},
	//						{1,-1},
	//						{1,0},
	//						{1,1}};//spoints=[-1 -1; -1 0; -1 1; 0 -1; -0 1; 1 -1; 1 0; 1 1];
	int xsize = img.cols;
	int ysize = img.rows;

	//int miny = -1;	//min(spoints(:,1));
	//int maxy = 1;	//max(spoints(:,1));
	//int minx = -1;	//min(spoints(:,2));
	//int maxx = 1;	//max(spoints(:,2));

	//int bsizey = 3;	//ceil(max(maxy,0))-floor(min(miny,0))+1;
	//int bsizex = 3;	//ceil(max(maxx,0))-floor(min(minx,0))+1;

	//int origy = 2;	//1-floor(min(miny,0));
	//int origx = 2;	//1-floor(min(minx,0));
	if (xsize < neighbors->bsizex || ysize < neighbors->bsizey){
		cerr << "Too small input image. Should be at least (2*radius+1) x (2*radius+1)" << endl;
		return(Mat());
	}

	int dx = xsize - neighbors->bsizex;
	int dy = ysize - neighbors->bsizey;
	Mat d_img;
	img.convertTo(d_img, CV_32FC1);
	Mat orig(img, cvRect(neighbors->origx-1, neighbors->origy-1, dx + 1, dy + 1));
	Mat bitResult(cvSize(dx + 1, dy + 1), CV_8UC1);
	Mat result(cvSize(dx + 1, dy + 1), CV_8UC1, cvScalar(0));

	for (int n = 0; n < neighbors->len; n++){
		double oy = neighbors->spoints.at<float>(n, 0) + neighbors->origy;
		double ox = neighbors->spoints.at<float>(n, 1) + neighbors->origx;

		int fx = floor(ox);
		int fy = floor(oy);

		int cx = ceil(ox);
		int cy = ceil(oy);

		int ry = floor(oy + 0.5);
		int rx = floor(ox + 0.5);
		if ((abs(ox - rx) < 1e-6) && (abs(oy - ry) < 1e-6)){
			Mat rotated(img,
				cvRect(rx - 1,
				ry - 1,
				dx + 1,
				dy + 1));
			
			compare(orig, rotated, bitResult, cv::CMP_LE);
		}
		else {
			double ty = oy - fy;
			double tx = ox - fx;

			//% Calculate the interpolation weights.
			double w1 = (1 - tx) * (1 - ty);
			double w2 = tx  * (1 - ty);
			double w3 = (1 - tx) *      ty;
			double w4 = tx  *      ty;
			//% Compute interpolated pixel values
			Mat a1, a2;

			addWeighted(Mat(d_img, cvRect(fx - 1, fy - 1, dx + 1, dy + 1)), w1, Mat(d_img, cvRect(cx - 1, fy - 1, dx + 1, dy + 1)), w2, 0, a1);
			addWeighted(Mat(d_img, cvRect(fx - 1, cy - 1, dx + 1, dy + 1)), w3, Mat(d_img, cvRect(cx - 1, cy - 1, dx + 1, dy + 1)), w4, 0, a2);
			add(a1, a2, a1);
			//N = w1*d_image(fy:fy+dy,fx:fx+dx) + w2*d_image(fy:fy+dy,cx:cx+dx) + ...
			//	w3*d_image(cy:cy+dy,fx:fx+dx) + w4*d_image(cy:cy+dy,cx:cx+dx);

			compare(Mat(d_img, cvRect(neighbors->origx - 1, neighbors->origy - 1, dx + 1, dy + 1)),
				a1, bitResult, cv::CMP_LE);
			//D = N >= d_C; 
		}
		int bit = 1 << n;
		bitwise_and(bitResult, bit, bitResult);
		bitwise_or(result, bitResult, result);
	}

	if (map){
		map->Map(&result);
	}
	
	return result;
}
Mat createHistogram8(Mat lbpEncodedImage,int bins,int horizontalParts, int verticalParts){
	int X = verticalParts;
	int Y = horizontalParts;
	
	//for(int my = 0; my<lbpEncodedImage.rows; my++)
	//	for(int mx = 0; mx<lbpEncodedImage.cols;mx++)
	//			cout << " " << (uint)(lbpEncodedImage.at<uchar>(mx,my));
	//	cout << endl;
	//	cout << "####################################" << endl;

	int width = lbpEncodedImage.cols;
	int height = lbpEncodedImage.rows;
	Mat result(cvSize(bins, X*Y), CV_32F);
	result.setTo(0);
	int cell_width = width / X + 1;
	int cell_height = height / Y + 1; 

	int tx = 0;
	for(int x = 0; x < X; x++){
		int ty = 0;
		int wx = x >= (width % X)  ? cell_width - 1: cell_width;

		for(int y = 0; y < Y; y++){
			
			int wy = y >= (height % Y) ? cell_height - 1: cell_height;
			int resultRow = x*Y + y;
			int res[256];
			memset(res,0,256*sizeof(int));
	
			for(int _x = tx; _x < tx+wx; _x++){
				for(int _y = ty; _y < ty+wy; _y++){
					uchar value = lbpEncodedImage.at<uchar>(_y, _x);
					//int r = result.at<int>(resultRow,value);
					result.at<float>(resultRow,value) = result.at<float>(resultRow,value)+1;
					res[value]++;
				}
			}
			
			ty += wy;
		}
		tx += wx;
	}
	//double max;
	//minMaxLoc(result,NULL,&max);
	//for(int i = 0; i < result.cols; i++){
	//	cout << " " << result.at<float>(0,i);
	//}
	//cout << endl;
	//cout << "---------------------------------"<<endl;
	return result;///(double)sum(result)[0];
}

bool filterTag = false;
std::string filteringTag = "";
int horizontalParts = 1;
int verticalParts = 1;
int radius = 1;
bool initialized = false;
bool normalize = true;
Mapping * mapping;

bool String2BoolConvert(string var, string desc, bool defaultVal)
{
	if (var == "true" )
		return true;
	else if (var == false)
		return false;
	cerr << "Unknown state of boolean variable(" << desc << "). Taking " << defaultVal << " as defaut.";

	return defaultVal;
}

bool LbpFEModule::ExtractFeatures(vector<Sample *> & rSamples){
	if (!initialized){
		if (mAttributes.find("filterTag") != mAttributes.end()){
			filterTag = true;
			filteringTag = mAttributes["filterTag"];
		}
		if (mAttributes.find("horizontal") != mAttributes.end()){
			horizontalParts = atoi(mAttributes["horizontal"].c_str());
		}
		if (mAttributes.find("vertical") != mAttributes.end()){
			verticalParts = atoi(mAttributes["vertical"].c_str());
		}
		if (mAttributes.find("radius") != mAttributes.end()){
			radius = atoi(mAttributes["radius"].c_str());
		}

		if (mAttributes.find("normalize") != mAttributes.end()){
			string s = mAttributes["normalize"];
			string positive = "true";
			string negatitive = "false";
			
		}


		mapping = new U8Mapping(8);
		initialized = true;
	}

	for (vector<Sample *>::iterator it = rSamples.begin();
		it != rSamples.end();
		it++){
		if (filterTag && !((*it)->HasMatrix(filteringTag))){
			continue;
		}

		Mat encoded_image = lbpmat((*it)->GetImage(), mapping, setNeighbors(radius, 8));
		(*it)->AddMatrix("features",
			createHistogram8(
			encoded_image,
			mapping->ResultLength(),
			horizontalParts,
			verticalParts));
	}
	return true;
}

int main (int argc, char ** argv){
	mapping = new U8Mapping(8);
		//struct lbpType * s =  setNeighbors(1,8);
		//Mapping * tmp = new NoMapping(8);
		//Mat encoded_image2 = lbpmat((*it)->GetImage(),tmp,setNeighbors(1,8));
		//createHistogram8(
		//						encoded_image2,
		//						tmp->ResultLength(),
		//						horizontalParts,
		//						verticalParts);

	Mat img = imread("1_1.bmp", 0);
	Mat encoded_image = lbpmat(img,mapping,setNeighbors(radius,8));//setOriginalNeighbors());
		
	Mat features = 
		createHistogram8(
			encoded_image,
			mapping->ResultLength(),
			horizontalParts,
			verticalParts);
	return 0;
}