/**
 * @file DynamicLoader.cpp
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#include <windows.h>
#include <strsafe.h>
#include <winbase.h>
#include <Imagehlp.h>
#include <stdio.h>
#include <io.h>
#include <time.h>
#include <fstream>

#include "base/DynamicLoader.h"

#define BIOSANDBOX_HOME_ENV_VAR "BIOSANDBOX_HOME"
#define MODULES_DIR "\\modules"

string getModuleTypeName(BaseModule::ModuleType type){
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

bool compare(const BaseModule * d1, const BaseModule * d2)
{
	return d1->getType() < d2->getType();
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
	this->mBiosandboxHomePath = getHome();
}

DynamicLibraryLoader::~DynamicLibraryLoader(){
	
}

const char * GetArchitectureString(WORD machine){
	switch (machine){
		case IMAGE_FILE_MACHINE_AMD64:
			return "x64 (64 bit)";
		case IMAGE_FILE_MACHINE_I386:
			return "x86 (32 bit)";
		case IMAGE_FILE_MACHINE_IA64:
			return "I64 (Itanium 64 bit)";
		default:
			return "(Unknown)";
	}
}

WORD GetExecutableArchitecture(){
	#ifdef _M_X64
		return IMAGE_FILE_MACHINE_AMD64;
	#endif
	#ifdef _M_IX86 
		return IMAGE_FILE_MACHINE_I386;
	#endif
	#ifdef _M_IA64
		return IMAGE_FILE_MACHINE_IA64;
	#endif
	return -1;
}

bool CheckArchitecture(string moduleFile, string modulePath){
	bool ret = false;

	

	PLOADED_IMAGE img = ImageLoad(moduleFile.c_str(), modulePath.c_str());
	if(img == NULL){
		cerr<<"Unable to check header of library "<< moduleFile <<" for architecture."<<endl;
		return false;
	}
	if( img->FileHeader->FileHeader.Machine == GetExecutableArchitecture()){
		ret = true;
	} else {
		cerr<<"Inconsistent architecture. Library "<< moduleFile << " is compiled for " << 
			GetArchitectureString(img->FileHeader->FileHeader.Machine) <<  
			" but Biosandbox is for "<< 
			GetArchitectureString(GetExecutableArchitecture()) << "." << endl;
		ret = false;
	}
	ImageUnload(img);
	return ret;
}

void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}


BaseModule * DynamicLibraryLoader::LoadModuleFrom(string moduleFile){
	WCHAR libraryPath[MAX_PATH];
	moduleFile += ( moduleFile.rfind(".dll") == moduleFile.length()-4 ) ? "" : ".dll";
	string moduleFullPath = mBiosandboxHomePath + MODULES_DIR + "\\" + moduleFile;
	//moduleFullPath += ( moduleFile.rfind(".dll") == moduleFile.length()-4 ) ? "" : ".dll";
	
	ifstream dllfile(moduleFullPath, std::ios::binary);
	if(!dllfile){
		cerr<<"Module file "<< moduleFullPath <<" does not exist or is not allowed to read!"<<endl;
		return 0;
	}
	dllfile.close();
	if(!CheckArchitecture(moduleFile, mBiosandboxHomePath + MODULES_DIR)){
		cerr << "There is a problem with architecture compatibility!" << endl;
		return 0;
	}
	
	MultiByteToWideChar( 0,0, moduleFullPath.c_str(), moduleFullPath.length()+1, libraryPath, MAX_PATH);
	
	HMODULE hModule = LoadLibrary(moduleFullPath.c_str()/*libraryPath*/);
	if (hModule == 0){
		cerr<<"Unable to load library "<< moduleFullPath<<endl;
		ErrorExit(TEXT("LoadLibrary"));
		//DWORD err = GetLastError();
		return 0;//exit(-1);
	}
	
	GMFF GetModuleFromFile = (GMFF)GetProcAddress(hModule, "GetModuleFromFile");
	if(GetModuleFromFile == 0){
		cerr<<"Library "<< moduleFullPath<<" doesn't contain any Biosandbox module or module is not visible."<<endl;
		return 0;//exit(-1);
	}

	BaseModule * module = GetModuleFromFile();
	if(module == 0){
		cerr<<"Unable to instantiate module "<< moduleFullPath << endl;
		exit(-1);
	}
	return module;
}


void DynamicLibraryLoader::listAllModules(){
	string modulesDir = getHome() + MODULES_DIR + "\\*.dll";
	vector<BaseModule *> allModules; 

	struct _finddata_t dllFile;
    long hFile;	
	
	DynamicLibraryLoader * dl = new DynamicLibraryLoader();
	if( (hFile = _findfirst(modulesDir.c_str() , &dllFile )) == -1L ){
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
   }
	/*sort(allModules.begin(),allModules.end(),compare);
	for(vector<BaseModule* >::iterator it = allModules.begin();
		it != allModules.end();
		it++){
		cout << "\t" << (*it)->name << " ("<< getModuleTypeName((*it)->getType()) <<")"<< endl;
	}*/
}