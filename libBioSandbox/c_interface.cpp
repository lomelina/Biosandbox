#include <iostream>
#include <set>
#include "base/WorkflowEngine.h"
#include "conf/XmlConfigurationLoader.h"
#include "InternalIO.h"
#include "IterativeWorkflowEngine.h"


extern "C"
{
	struct  NativeEngine
    {
        IterativeWorkflowEngine * engine;
        InternalInput * inputModule;
        InternalOutput * outputModule;
    };

	__declspec(dllexport)
	bool __stdcall ProcessImage(NativeEngine engine, char pixelData[], int width, int height, Coordinates2D faces[], int numFaces, int identities[])
    {	
		//cout << "Processing ... image: w " << width << ", h " << height <<  " Faces: " << numFaces << endl;
		if(!engine.engine){
			cerr << "Unable to process image. Biosandbox was not initialized!" << endl;
			return false;
		}

		if(engine.inputModule != 0) 
			engine.inputModule->SetImage(pixelData,width,height,faces,numFaces);
		engine.engine->Iterate();
		if(engine.outputModule != 0) 
			engine.outputModule->GetIdentities(identities);
		return true;
    };

	__declspec(dllexport)
	bool __stdcall ProcessImageWithDistances(NativeEngine engine, char pixelData[], int width, int height, Coordinates2D faces[], int numFaces, int identities[], int numDistances, double distances[])
    {	
		//cout << "Processing (with distances) ... image: w " << width << ", h " << height <<  " Faces: " << numFaces << endl;
		if(!engine.engine){
			cerr << "Unable to process image. Biosandbox was not initialized!" << endl;
			return false;
		}

		if(engine.inputModule != 0) 
			engine.inputModule->SetImage(pixelData,width,height,faces,numFaces);
		engine.engine->Iterate();
		if(engine.outputModule != 0){
			engine.outputModule->GetIdentities(identities);
			engine.outputModule->GetDistances(distances,numDistances);
		}
		return true;
    };

	__declspec(dllexport)
	bool __stdcall ProcessImageWithDepth(NativeEngine engine, 
						char pixelData[], int width, int height, 
						unsigned short depthPixelData[], int depthWidth, int depthHeight, 
						Coordinates2D faces[], int numFaces, int identities[], int numDistances, double distances[])
    {	
		//cout << "Processing (with distances) ... image: w " << width << ", h " << height <<  " Faces: " << numFaces << endl;
		if(!engine.engine){
			cerr << "Unable to process image. Biosandbox was not initialized!" << endl;
			return false;
		}

		if(engine.inputModule != 0){
			engine.inputModule->SetImage(pixelData,width,height,faces,numFaces);
			engine.inputModule->SetDepthMap(depthPixelData,depthWidth,depthHeight);
		}
		engine.engine->Iterate();
		if(engine.outputModule != 0){
			engine.outputModule->GetIdentities(identities);
			engine.outputModule->GetDistances(distances,numDistances);
			return engine.outputModule->IsInteracting() != 0;
		}
		return false;
    };

	__declspec(dllexport)
	NativeEngine __stdcall InitializeBiosandbox(const char* configFile)
    {	
		NativeEngine engine;
		engine.engine = 0;
		engine.inputModule = 0;
		engine.outputModule = 0;
		
		XmlConfigurationLoader * loader = new XmlConfigurationLoader();
	
		Configuration * conf = loader->LoadForm(configFile);
		
		delete loader;

		if(conf == 0){
			std::cerr << "ERROR: Unable to initialize biosandbox. Assigned configuration (" << configFile << ") is not valid XML file!" << endl;
			return engine;
		}

		if(conf->InputModulesCount() == 0){
			std::cout << "INFO: Connecting .NET application to the input of Biosandbox system..." << endl;
			conf->AddInputModule(engine.inputModule = new InternalInput());
		}

		if(conf->FinishingModulesCount() <= 1){
			std::cout << "INFO: Connecting output from Biosandbox system to .NET application..." << endl;
			conf->AddFinishingModule(engine.outputModule = new InternalOutput());
		}

		engine.engine = new IterativeWorkflowEngine(conf);
		if(!engine.engine->IsValid()){
			delete engine.engine;
			std::cerr << "ERROR: Unable to initialize biosandbox. Assigned configuration has not valid workflow!" << endl;
			return engine;
		}
		return engine;
    };

	__declspec(dllexport)
	void __stdcall DeinitializeBiosandbox(NativeEngine engine)
    {	
		delete engine.engine;
    };
}