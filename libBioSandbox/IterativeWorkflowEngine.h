#pragma once
#include "..\biosandbox\src\base\workflowengine.h"
class IterativeWorkflowEngine :
	public WorkflowEngine
{
public:
	IterativeWorkflowEngine(Configuration * currentConfig);
	~IterativeWorkflowEngine(void);
	inline bool IsValid() { return mConfigurationValid; }
	bool Iterate();
private:
	Configuration * mCurrentConfig;
	bool mConfigurationValid;
};

