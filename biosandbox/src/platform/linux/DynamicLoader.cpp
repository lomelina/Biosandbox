#include <dlfcn.h>	
#include <dirent.h>
#include <sys/types.h>

#include "base/DynamicLoader.h"

#define BIOSANDBOX_HOME_ENV_VAR "BIOSANDBOX_HOME"
#define MODULES_DIR "\\modules"

string getModuleName(BaseModule::ModuleType type){
	switch(type){
		case BaseModule::INPUT:
			return "Input module";
		case BaseModule::PREPROCESSING:
			return "Preprocessing module";
		case BaseModule::FEATURE_EXTRACTION:
			return "Features extraction module";
		case BaseModule::POSTPROCESSING:
			return "Postprocessing module";
		case BaseModule::FINISHING:
			return "Finishing module";
		default:
			return "Unknown module";
	}
}

string getHome(){
	char * cHomePath = getenv(BIOSANDBOX_HOME_ENV_VAR);
	if(cHomePath){
		return string(cHomePath);
	} else {
		cerr << "warning: environment variable "<<BIOSANDBOX_HOME_ENV_VAR<<" is not defined." << endl;
		return "";
	}
}

DynamicLibraryLoader::DynamicLibraryLoader(){

}

DynamicLibraryLoader::~DynamicLibraryLoader(){

}

BaseModule * DynamicLibraryLoader::LoadModuleFrom(string moduleName){
	string moduleFile= "lib" + moduleName + ".so";
	void *hndl = dlopen(moduleFile.c_str(), RTLD_NOW);
	if(hndl == NULL){
		cerr << dlerror() << endl;
		return(NULL);
	}
	GMFF GetModuleFromFile = (GMFF)dlsym(hndl, "GetModuleFromFile");

	if(GetModuleFromFile == 0){
		cerr<<"Library "<< moduleFile<<" doesn't contain any Biosandbox module or module is not visible."<<endl;
		return 0;
	}

	BaseModule * module = GetModuleFromFile();
	if(module == 0){
		return 0;
	}
	return module;
}

void DynamicLibraryLoader::listAllModules(){
	string strModulesDir = getHome() + MODULES_DIR + "\\*.dll";
	DIR *modulesDir = opendir(strModulesDir.c_str());
	DynamicLibraryLoader * dl = new DynamicLibraryLoader();
	if(modulesDir)
	{
		struct dirent *ent;
		while((ent = readdir(modulesDir)) != NULL){
			BaseModule * module;
			if((module = dl->LoadModuleFrom(ent->d_name)) != 0) {
				cout << "\t" << ent->d_name << "\t ("<< getModuleName(module->getType()) <<")"<< endl;
			}
		}
	}
}
