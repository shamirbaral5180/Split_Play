#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace SplitPlayHost
{

struct DeviceInfo
{
	std::wstring name;
	unsigned int handle = 0;
	bool isKeyboard = false;
};

struct RunningProcessInfo
{
	unsigned long pid = 0;
	std::wstring name;
	std::wstring windowTitle;
	bool hasWindow = false;
};

// A friendly name for a HID device, keyed by its VID/PID
struct HidFriendlyName
{
	std::wstring vidPid;       // e.g. "VID_046D&PID_C534"
	std::wstring friendlyName; // e.g. "Logitech USB Mouse"
	std::wstring description;  // e.g. "USB Input Device"
};

// Enumerates HID devices and their friendly names (via SetupAPI)
std::vector<HidFriendlyName> EnumerateHidFriendlyNames();

// Extracts the "VID_xxxx&PID_yyyy" token from a raw input device path (empty if not found)
std::wstring ExtractVidPid(const std::wstring& devicePath);

// Looks up a friendly name for a raw input device path (returns empty if unknown)
std::wstring LookupDeviceFriendlyName(const std::wstring& devicePath);

// Enumerates raw input keyboards and mice connected to the system
std::vector<DeviceInfo> EnumerateInputDevices(bool keyboards);

// Enumerates processes that have a visible top-level window (i.e. running apps/games)
std::vector<RunningProcessInfo> EnumerateWindowedProcesses();

// ---- Live input activity tracking (used to highlight which physical device is which) ----

// Starts/stops a background thread that watches for the most recently used input device.
void StartInputActivityMonitor();
void StopInputActivityMonitor();

// Returns the raw input device handle of the most recently used mouse/keyboard.
// Returns 0 if nothing has been used yet.
unsigned int GetLastActiveMouse();
unsigned int GetLastActiveKeyboard();

// True if the given device handle was the most recently used one (within the glow window).
bool IsDeviceActive(unsigned int handle);

}
