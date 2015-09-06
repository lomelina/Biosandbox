/**
 * @file WorkflowEngine.cpp
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#include <sys/timeb.h>
#include "WorkflowEngine.h"

#undef _DEBUG
unsigned long getMilliCount(){
	timeb tb;
	ftime(&tb);
	unsigned long nCount = tb.millitm;
	nCount += (tb.time & 0xfffff) * 1000;
	return nCount;
}

WorkflowEngine::WorkflowEngine(void)
{
}

WorkflowEngine::~WorkflowEngine(void)
{

}

bool WorkflowEngine::ValidateConfiguration(Configuration * currentConfig){
	return true;
}

bool WorkflowEngine::StartProcessLoop(Configuration * currentConfig){
	
	bool isFinished = false;

	if(this->ValidateConfiguration(currentConfig) == false){
		cerr<<"Config. validation failed. Check modules order!"<< endl;
		return false;
	}

		while(!isFinished){
#if _DEBUG
		unsigned long start = getMilliCount();
#endif
		vector<Sample *> samples;
		{
			InputModule * module = currentConfig->GetInputModule();
			isFinished = module->Read(samples);
		}

#if _DEBUG
		unsigned long input = getMilliCount();
#endif
		for(int i=0; i< currentConfig->PreprocessingModulesCount();i++ ){
			PreprocessingModule * module = currentConfig->GetPreprocessingModule(i);
			module->Process(samples);
		}

#if _DEBUG
		unsigned long preprocessing = getMilliCount();
#endif
		{
			FeatureExtractionModule * module = currentConfig->GetFeatureExtractionModule();
			if(module != NULL){
				module->ExtractFeatures(samples);
			}
		}

#if _DEBUG
		unsigned long featureExtraction = getMilliCount();
#endif
		for(int i=0; i< currentConfig->PostprocessingModulesCount();i++ ){
			PostprocessingModule * module = currentConfig->GetPostprocessingModule(i);
			isFinished |= module->Process(samples);
		}

#if _DEBUG
		unsigned long postprocessing = getMilliCount();
#endif
		{
			FinishingModule * module = currentConfig->GetFinishingModule();
			if(module != NULL){
				isFinished |= module->Finish(samples);
			}
		}

#if _DEBUG
		unsigned long finishing = getMilliCount();
#endif
		// release all samples
		for(	vector<Sample* >::iterator it = samples.begin(); 
				it != samples.end(); 
				it++){
			delete *it;
		}
#if _DEBUG
		cout << "IN: " << (input - start) <<
			", PRE: " << (preprocessing - input) <<
			", FE: " << (featureExtraction - preprocessing) <<
			", POST: " << (postprocessing - featureExtraction) <<
			", FIN: " << (finishing - postprocessing) <<
			" (TOTAL: " << (finishing - start) << ")" << endl;
#endif
	}
	return true;
}

