#include "IterativeWorkflowEngine.h"


IterativeWorkflowEngine::IterativeWorkflowEngine(Configuration * currentConfig)
{
	if(this->ValidateConfiguration(currentConfig)){
		mConfigurationValid = true;
		mCurrentConfig = currentConfig;
	} else {
		mConfigurationValid = false;
		mCurrentConfig = 0;
	}
}


IterativeWorkflowEngine::~IterativeWorkflowEngine(void)
{
}


bool IterativeWorkflowEngine::Iterate(){
	
	bool isFinished = false;
	vector<Sample *> samples;
	{
		InputModule * module = mCurrentConfig->GetInputModule();
		if(module != NULL){
			isFinished = module->Read(samples);
		}
	}

	for(int i=0; i< mCurrentConfig->PreprocessingModulesCount();i++ ){
		PreprocessingModule * module = mCurrentConfig->GetPreprocessingModule(i);
		module->Process(samples);
	}

	{
		FeatureExtractionModule * module = mCurrentConfig->GetFeatureExtractionModule();
		if(module != NULL){
			module->ExtractFeatures(samples);
		}
	}

	for(int i=0; i< mCurrentConfig->PostprocessingModulesCount();i++ ){
		PostprocessingModule * module = mCurrentConfig->GetPostprocessingModule(i);
		module->Process(samples);
	}


	{
		FinishingModule * module = mCurrentConfig->GetFinishingModule();
		if(module != NULL){
			isFinished |= module->Finish(samples);
		}
	}
	
	// release all samples
	for(	vector<Sample* >::iterator it = samples.begin(); 
			it != samples.end(); 
			it++){
		delete *it;
	}
	
	return isFinished;
}

