#include "Loader.h"

void EnableDisableMessageFilter(SplitPlayInstanceHandle instanceHandle, SplitPlayMessageFilterIDs filterID, bool enable)
{
	if (const auto find = SplitPlay::instances.find(instanceHandle); find != SplitPlay::instances.end())
	{
		auto& instance = find->second;

		WaitClientConnect(instance);

		SplitPlayPipe::PipeMessageSetupMessageFilter message
		{
			filterID,
			enable
		};

		SplitPlaySendPipeMessage(instance.pipeHandle, SplitPlayPipe::PipeMessageType::SetupMessageFilter, &message);
	}
}

extern "C" __declspec(dllexport) void EnableMessageFilter(SplitPlayInstanceHandle instanceHandle, SplitPlayMessageFilterIDs filterID)
{
	EnableDisableMessageFilter(instanceHandle, filterID, true);
}

extern "C" __declspec(dllexport) void DisableMessageFilter(SplitPlayInstanceHandle instanceHandle, SplitPlayMessageFilterIDs filterID)
{
	EnableDisableMessageFilter(instanceHandle, filterID, false);
}

void EnableDisableMessageBlock(SplitPlayInstanceHandle instanceHandle, unsigned int messageID, bool block)
{
	if (const auto find = SplitPlay::instances.find(instanceHandle); find != SplitPlay::instances.end())
	{
		auto& instance = find->second;

		WaitClientConnect(instance);

		SplitPlayPipe::PipeMessageSetupMessageBlock message
		{
			messageID,
			block
		};

		SplitPlaySendPipeMessage(instance.pipeHandle, SplitPlayPipe::PipeMessageType::SetupMessageBlock, &message);
	}
}

extern "C" __declspec(dllexport) void EnableMessageBlock(SplitPlayInstanceHandle instanceHandle, unsigned int messageID)
{
	EnableDisableMessageBlock(instanceHandle, messageID, true);
}

extern "C" __declspec(dllexport) void DisableMessageBlock(SplitPlayInstanceHandle instanceHandle, unsigned int messageID)
{
	EnableDisableMessageBlock(instanceHandle, messageID, false);
}
