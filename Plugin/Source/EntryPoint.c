#include "core/runtime.h"

OnStart(1)
{
	fprintf(stdout, "Plugin OnStart ran.\n");
}

OnUpdate(1)
{
	fprintf(stdout, "Plugin OnUpdate ran.\n");
}

AfterUpdate(1)
{
	fprintf(stdout, "Plugin AfterUpdate ran.\n");
}

OnClose(1)
{
	fprintf(stdout, "Plugin OnClose ran.\n");
}