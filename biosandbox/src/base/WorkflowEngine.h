/**
 * @file WorkflowEngine.h
 * @author  Lubos Omelina <lomelina@gmail.com>
 * 
 *
 */

#ifndef WORKFLOWENGINE_H_
#define WORKFLOWENGINE_H_

#include "Configuration.h"

unsigned long getMilliCount();

class WorkflowEngine
{
public:
	WorkflowEngine(void);
	~WorkflowEngine(void);

	bool StartProcessLoop(Configuration * currentConfig);
	
protected:
	bool ValidateConfiguration(Configuration * currentConfig);
	
};


#endif //WORKFLOWENGINE_H_