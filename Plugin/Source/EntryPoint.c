#include "core/runtime.h"
#include "core/tasks.h"

OnStart(1)
{
	fprintf_green(stdout, "Plugin OnStart ran: %i\n", Tasks.ThreadId());
}

int i = 0;
int x = 0;

OnUpdate(1)
{
	fprintf_green(stdout, "Plugin OnUpdate ran: %i\n", i++);
}

AfterUpdate(1)
{
	fprintf_green(stdout, "Plugin AfterUpdate ran: %i\n", x++);
}

OnClose(1)
{
	fprintf_green(stdout, "Plugin OnClose ran: %i\n", Tasks.ThreadId());
}