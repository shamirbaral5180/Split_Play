#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace SplitPlayHost
{

struct MonitorInfo
{
	HMONITOR handle = nullptr;
	std::wstring deviceName;   // e.g. "\\\\.\\DISPLAY2"
	std::wstring friendlyName; // e.g. "Spacedesk Display"
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool isPrimary = false;
};

std::vector<MonitorInfo> EnumerateMonitors();

MonitorInfo GetMonitorForWindow(HWND hwnd);

std::string MonitorLabel(const MonitorInfo& monitor);

}
