#include "Loader.h"

void InstallUninstallHook(SplitPlayInstanceHandle instanceHandle, SplitPlayHookIDs hookID, bool install)
{
	if (const auto find = SplitPlay::instances.find(instanceHandle); find != SplitPlay::instances.end())
	{
		auto& instance = find->second;

		WaitClientConnect(instance);

		SplitPlayPipe::PipeMessageSetupHook message
		{
			hookID,
			install
		};

		SplitPlaySendPipeMessage(instance.pipeHandle, SplitPlayPipe::PipeMessageType::SetupHook, &message);
	}
}

extern "C" __declspec(dllexport) void InstallHook(SplitPlayInstanceHandle instanceHandle, SplitPlayHookIDs hookID)
{
	InstallUninstallHook(instanceHandle, hookID, true);
}

extern "C" __declspec(dllexport) void UninstallHook(SplitPlayInstanceHandle instanceHandle, SplitPlayHookIDs hookID)
{
	InstallUninstallHook(instanceHandle, hookID, false);
}