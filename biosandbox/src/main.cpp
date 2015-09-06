/**
 * @file main.cpp
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */


#include <iostream>
#include <set>
#include "base/WorkflowEngine.h"
#include "conf/XmlConfigurationLoader.h"

void print_usage(){
	cerr<< "Usage:"<<endl;
	cerr<< "\tbiosandbox configuration.xml"<<endl;	
}

int main(int argc, char ** argv){
	string conf_file = "";
	set<string> arguments;
	cout<<"Parsing arguments...\n"<<endl;
	
	for(int i=1; i<argc; i++){
		string parameter = string(argv[i]);
		if(parameter.at(0) == '-')
			arguments.insert(parameter);
		else
			conf_file = parameter;
	}

	if(arguments.find("-l") != arguments.end()){
		DynamicLibraryLoader::listAllModules();
		exit(0);
	}


	if(conf_file == ""){
		cerr<< "No configuration file defined!"<<endl;
		print_usage();
		return 0;
	}

	XmlConfigurationLoader * loader = new XmlConfigurationLoader();
	
	Configuration * conf = loader->LoadForm(conf_file);
	if(conf == 0){
		cerr<< "Unable to load configuration file: "<< conf_file << endl;
		return 0;
	}

	WorkflowEngine * we = new WorkflowEngine();
	we->StartProcessLoop(conf);
	
	return 0;
}