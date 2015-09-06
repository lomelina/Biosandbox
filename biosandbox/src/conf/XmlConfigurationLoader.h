/**
 * @file XmlConfigurationLoader.h
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#pragma once
#include "base/Configuration.h"
#include "base/DynamicLoader.h"

class XmlConfigurationLoader : public ConfigurationLoader
{
public:
	XmlConfigurationLoader();
	~XmlConfigurationLoader();
	Configuration * LoadForm(string source);
private:
	DynamicLibraryLoader * mDl;
	vector<BaseModule *> LoadModules(string type,void * docHandle);
	BaseModule * LoadModule(void * moduleElement);
};
