#pragma once
#include "splitplayloader.h"
#include <windows.h>
#include <iostream>
#include "TrackedInstances.h"
#include "pipeinclude.h"

bool Isx64(unsigned long pid);

SplitPlayInstanceHandle CreateInstanceHandle(unsigned long pid);

void WaitClientConnect(SplitPlay::SplitPlayInstance& instance);

template<typename Body>
void SplitPlaySendPipeMessage(HANDLE pipe, SplitPlayPipe::PipeMessageType messageType, Body* body)
{
	SplitPlayPipe::PipeMessageHeader header
	{
		messageType,
		sizeof(Body)
	};

	DWORD numBytesWritten = 0;
	BOOL write = WriteFile(
		pipe,
		&header,
		sizeof(SplitPlayPipe::PipeMessageHeader),
		&numBytesWritten,
		NULL
	);

	if (write)
	{
		// std::cout << "Successfully sent message header" << std::endl;
	}
	else
		std::cerr << "Failed to send message header" << std::endl;

	write = WriteFile(
		pipe,
		body,
		sizeof(Body),
		&numBytesWritten,
		NULL
	);

	if (write)
	{
		// std::cout << "Successfully sent message body" << std::endl;
	}
	else
		std::cerr << "Failed to send message body" << std::endl;
}