/**
 * @file XmlConfigurationLoader.cpp
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#include "tinyxml.h"

#include "XmlConfigurationLoader.h"


XmlConfigurationLoader::XmlConfigurationLoader(void)
{
	mDl = new DynamicLibraryLoader();
}

XmlConfigurationLoader::~XmlConfigurationLoader(void)
{
}

Configuration * XmlConfigurationLoader::LoadForm(string source){
	Configuration * conf = 0;
	TiXmlDocument doc( source.c_str() );
	if(doc.LoadFile() == false){
		cerr<<"Configuration file "<< source << " not found!"<<endl;
		return 0;
	}	
	
	TiXmlHandle doc_handle( &doc );

	conf = new Configuration(
			XmlConfigurationLoader::LoadModules("Input",&doc_handle),
			XmlConfigurationLoader::LoadModules("Preprocessing",&doc_handle),
			XmlConfigurationLoader::LoadModules("FeaturesExtraction",&doc_handle),
			XmlConfigurationLoader::LoadModules("Postprocessing",&doc_handle),
			XmlConfigurationLoader::LoadModules("Finishing",&doc_handle));

	return conf; 
}

vector<BaseModule *> XmlConfigurationLoader::LoadModules(string type,void * docHandle){
	vector<BaseModule *> modules;
	TiXmlHandle * handle = (TiXmlHandle *) docHandle;
	TiXmlElement * elem = handle->FirstChild("Configuration").FirstChild(type.c_str()).FirstChild("Module").ToElement();
	if(elem == 0){
		return modules;
	}
	do{
		BaseModule * m = XmlConfigurationLoader::LoadModule(elem);	
		if(m != 0){
			modules.push_back(m);
		}
		elem = elem->NextSiblingElement("Module");
	} while(elem != 0);
	return modules;
}

BaseModule * XmlConfigurationLoader::LoadModule(void * module){
	TiXmlElement * elem = (TiXmlElement * )module;
	TiXmlAttribute * attr = elem->FirstAttribute();
	if(attr == 0){
		return 0;
	}
	map<string, string> parameters_map;
	while(attr){
		
		parameters_map.insert(	pair<string,string>(
									string(attr->Name()),
									string(attr->Value())
								)		
							);
		attr = attr->Next();
	}

	BaseModule * loaded_module = mDl->LoadModuleFrom(parameters_map[string("src")]);
	if (loaded_module != 0){
		for (map<string, string>::iterator it = parameters_map.begin();
			it != parameters_map.end();
			it++){
			loaded_module->AddAttribute(it->first, it->second);
		}
	}
	return loaded_module;
}
