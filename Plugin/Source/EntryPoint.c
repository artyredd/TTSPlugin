#include "core/runtime.h"

OnStart(0)
{
	fprintf(stdout, "Plugin OnStart ran.");
}

OnUpdate(0)
{
	fprintf(stdout, "Plugin OnUpdate ran.");
}

AfterUpdate(0)
{
	fprintf(stdout, "Plugin AfterUpdate ran.");
}

OnClose(0)
{
	fprintf(stdout, "Plugin OnClose ran.");
}