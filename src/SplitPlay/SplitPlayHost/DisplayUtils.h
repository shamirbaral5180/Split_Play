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

// ---- Identify helper: flash a big number/name on a specific monitor ----------
// Shows a large, topmost overlay on the chosen monitor for a couple of seconds so
// the user can see which physical screen a row refers to.
void FlashMonitorNumber(const MonitorInfo& monitor, int displayNumber);

// Clears any active monitor flash immediately.
void ClearMonitorFlash();

}
