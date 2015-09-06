/**
 * Copyright 2011 Lubos Omelina <lomelina@gmail.com>


 * @file Configuration.h
 * @author  Lubos Omelina <lomelina@gmail.com>
 *
 *
 */


#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "Modules.h"

class Configuration
{
public:
	Configuration(void);
	Configuration(	vector<BaseModule *> input,
					vector<BaseModule *> preprocessing,
					vector<BaseModule *> featuresExtraction,
					vector<BaseModule *> postprocessing,
					vector<BaseModule *> finishing)
								: 
							mInputModules(input),
							mPreprocessingModules(preprocessing),
							mFeatureExtractionModules(featuresExtraction),
							mPostprocessingModules(postprocessing),
							mFinishingModules(finishing) {};
	~Configuration(void);
	
	size_t InputModulesCount(){ return mInputModules.size();};
	void AddInputModule(InputModule * module){
		mInputModules.push_back(module);
	}

	size_t PreprocessingModulesCount(){ return mPreprocessingModules.size();};

	PreprocessingModule * GetPreprocessingModule(unsigned int index){
		if(index < mPreprocessingModules.size()){
			return dynamic_cast<PreprocessingModule *>(mPreprocessingModules[index]);
		}
		return 0;
	};

	size_t PostprocessingModulesCount(){ return mPostprocessingModules.size();};

	PostprocessingModule * GetPostprocessingModule(unsigned int index){
		if(index < mPostprocessingModules.size()){
			return dynamic_cast<PostprocessingModule *>(mPostprocessingModules[index]);
		}
		return 0;
	};

	InputModule * GetInputModule(){
		if(mInputModules.size()==0){
			return 0;
		}
		return dynamic_cast<InputModule *>(mInputModules[0]);
	};

	FeatureExtractionModule * GetFeatureExtractionModule(){
		if(mFeatureExtractionModules.size()==0){
			return 0;
		}
		return dynamic_cast<FeatureExtractionModule *>(mFeatureExtractionModules[0]);
	};

	FinishingModule * GetFinishingModule(){
		if(mFinishingModules.size()==0){
			return 0;
		}
		return dynamic_cast<FinishingModule *>(mFinishingModules[0]);
	};

	size_t FinishingModulesCount(){ return mFinishingModules.size();};
	void AddFinishingModule(FinishingModule * module){
		mFinishingModules.push_back(module);
	}


private:
	vector<BaseModule *> mInputModules;
	vector<BaseModule *> mPreprocessingModules;
	vector<BaseModule *> mFeatureExtractionModules;
	vector<BaseModule *> mPostprocessingModules;
	vector<BaseModule *> mFinishingModules;
};

class ConfigurationLoader	{
public:
	ConfigurationLoader() {};
	virtual ~ConfigurationLoader() {};
	virtual Configuration * LoadForm(string source)=0;
};

#endif // CONFIGURATION_H_