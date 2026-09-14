#include "splitplayloader.h"
#include "Loader.h"
#include <string>
#include "TrackedInstances.h"
#include <iostream>
#include "pipeinclude.h"

std::wstring GetPipeName(unsigned long pid)
{
	return std::wstring{ LR"(\\.\pipe\)" } + std::to_wstring(pid);
}

void CreateSplitPlayNamedPipe(SplitPlay::SplitPlayInstance& instance)
{
	auto pipeName = GetPipeName(instance.pid);
	HANDLE handle = CreateNamedPipeW(pipeName.c_str(),
									 PIPE_ACCESS_OUTBOUND,
									 PIPE_TYPE_BYTE,
									 PIPE_UNLIMITED_INSTANCES,
									 32,
									 32,
									 0,
									 NULL);

	if (handle == INVALID_HANDLE_VALUE)
		std::cerr << "Couldn't open named pipe" << std::endl;
	else
	{
		instance.pipeHandle = handle;
	}
}

SplitPlayInstanceHandle CreateInstanceHandle(unsigned long pid)
{
	auto handle = SplitPlay::instanceCounter++;
	SplitPlay::SplitPlayInstance instance{};
	instance.pid = pid;
	CreateSplitPlayNamedPipe(instance);
	SplitPlay::instances[handle] = instance;
	return handle;
}

void WaitClientConnect(SplitPlay::SplitPlayInstance& instance)
{
	if (!instance.clientConnected)
	{
		std::cout << "Starting named pipe wait" << std::endl;

		int count = 0;

		while (count < 10)
		{
			//FIXME: This needs a timeout
			if (ConnectNamedPipe(instance.pipeHandle, NULL))
			{
				std::cout << "Connected named pipe to pid " << instance.pid << std::endl;
				instance.clientConnected = true;
				return;
			}
			else
			{
				std::cerr << "Couldn't connect named pipe to pid " << instance.pid << ", waiting 5s" << std::endl;
				count++;
				Sleep(5000);
			}
		}
	}
}