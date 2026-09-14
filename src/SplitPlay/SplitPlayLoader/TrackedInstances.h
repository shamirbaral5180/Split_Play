#pragma once
#include <unordered_map>
#include "include/splitplayloader.h"
#include <windows.h>

namespace SplitPlay
{

class SplitPlayInstance
{
public:
	HANDLE pipeHandle = nullptr;
	bool clientConnected = false;
	unsigned long pid = 0;
};

extern std::unordered_map<SplitPlayInstanceHandle, SplitPlayInstance> instances;
extern SplitPlayInstanceHandle instanceCounter;

}