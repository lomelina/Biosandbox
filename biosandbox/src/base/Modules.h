/**
 * @file Modules.h
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#ifndef PROPERTY_H_
#define PROPERTY_H_


#include "Sample.h"

class Property{
private:
	string name;
};



class BaseModule{
public:

	enum ModuleType{
		INPUT = 0,
		PREPROCESSING,
		FEATURE_EXTRACTION,
		POSTPROCESSING,
		FINISHING
	};


	BaseModule(ModuleType t): mType(t) {};
	virtual ~BaseModule() {};


	ModuleType getType() const {return mType;};

	vector<Property> GetGeneratedProperties();
	
	virtual void OnCreate() {};

	bool Validate(vector<Property> * m);

	void AddAttribute(string name, string value){
		if(mAttributes.find(name) == mAttributes.end())
			mAttributes.insert(pair<string, string>(name,value));
	};

protected:
	map<string,string> mAttributes;
	vector<string> mPossibleAttributes;
private:
	ModuleType mType;
	string mName;

};


class InputModule: public BaseModule{
public: 
	InputModule() : BaseModule(BaseModule::INPUT) {};
	virtual bool Read(vector<Sample *> & rSamplesToProcess) = 0;
};

class PreprocessingModule: public BaseModule{
public: 
	PreprocessingModule() : BaseModule(BaseModule::PREPROCESSING) {};
	virtual bool Process(vector<Sample *> & rSamplesToProcess) = 0;
};

class FeatureExtractionModule: public BaseModule{
public: 
	FeatureExtractionModule() : BaseModule(BaseModule::FEATURE_EXTRACTION) {};
	virtual bool ExtractFeatures(vector<Sample *> & rSamplesToProcess) = 0;
};

class PostprocessingModule: public BaseModule{
public: 
	PostprocessingModule() : BaseModule(BaseModule::POSTPROCESSING) {};
	virtual bool Process(vector<Sample *> & rSamplesToProcess) = 0;
};

class FinishingModule: public BaseModule{
public: 
	FinishingModule() : BaseModule(BaseModule::FINISHING) {};
	virtual bool Finish(vector<Sample *> & rSamplesToProcess) = 0;
};


#if defined(WIN32)
	#define LIBRARY_FUNC_PREFIX _declspec(dllexport) 
#else	
	#define LIBRARY_FUNC_PREFIX
#endif
extern "C" LIBRARY_FUNC_PREFIX BaseModule * GetModuleFromFile();


typedef BaseModule * (*GMFF)();


#define INTERN_LIBRARY_INTERFACE(MODULE_NAME) \
extern "C" LIBRARY_FUNC_PREFIX BaseModule * GetModuleFromFile() {\
		return new MODULE_NAME();\
	}

#define INPUT_MODULE( MODULE_NAME ) \
	class MODULE_NAME : public InputModule {\
		public:\
			MODULE_NAME(){};\
			~MODULE_NAME(){};\
			bool Read(vector<Sample *> & rSamplesToProcess);\
	};\
	INTERN_LIBRARY_INTERFACE( MODULE_NAME )

#define PREPROCESSING_MODULE( MODULE_NAME ) \
	class MODULE_NAME : public PreprocessingModule {\
		public:\
			MODULE_NAME(){};\
			~MODULE_NAME(){};\
			bool Process(vector<Sample *> & rSamplesToProcess);\
	};\
	INTERN_LIBRARY_INTERFACE( MODULE_NAME )

#define POSTPROCESSING_MODULE( MODULE_NAME ) \
	class MODULE_NAME : public PostprocessingModule {\
		public:\
			MODULE_NAME(){};\
			~MODULE_NAME(){};\
			bool Process(vector<Sample *> & rSamplesToProcess);\
	};\
	INTERN_LIBRARY_INTERFACE( MODULE_NAME )


#define FEATURESEXTRACTION_MODULE( MODULE_NAME ) \
	class MODULE_NAME : public FeatureExtractionModule {\
		public:\
			MODULE_NAME(){};\
			~MODULE_NAME(){};\
			bool ExtractFeatures(vector<Sample *> & rSamplesToProcess);\
	};\
	INTERN_LIBRARY_INTERFACE( MODULE_NAME )

#define FINISHING_MODULE( MODULE_NAME ) \
	class MODULE_NAME : public FinishingModule {\
		public:\
			MODULE_NAME(){};\
			~MODULE_NAME(){};\
			bool Finish(vector<Sample *> & rSamplesToProcess);\
	};\
	INTERN_LIBRARY_INTERFACE( MODULE_NAME )


#endif // PROPERTY_H_
