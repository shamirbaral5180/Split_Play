#include "Cleanup.h"
#include <cstdio>
#include "FocusMessageLoop.h"

void SplitPlay::SplitPlayPerformCleanup()
{
	printf("SplitPlay Input performing shutdown cleanup\n");

	FocusMessageLoop::Cleanup();
}
