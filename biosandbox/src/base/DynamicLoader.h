/**
 * @file DynamicLoader.h
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#ifndef DYNAMICLIBRARYLOADER_H_
#define DYNAMICLIBRARYLOADER_H_

#include "Modules.h"

using namespace std;




class DynamicLibraryLoader{
public:
	DynamicLibraryLoader();
	~DynamicLibraryLoader();

	static void listAllModules();

	BaseModule * LoadModuleFrom(string moduleFile);
private: 
	string mBiosandboxHomePath;
};

#endif // DYNAMICLIBRARYLOADER_H_;