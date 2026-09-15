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

// A camera or other imaging device
struct CameraInfo
{
	std::wstring name;          // friendly name, e.g. "Web Camera"
	std::wstring deviceId;      // PnP device instance id
	bool inUse = false;         // currently being used by some app
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

// Enumerates cameras / imaging devices, flagging which are currently in use.
std::vector<CameraInfo> EnumerateCameras();

// Enumerates processes that have a visible top-level window (i.e. running apps/games)
std::vector<RunningProcessInfo> EnumerateWindowedProcesses();

// ---- Live input activity tracking (used to highlight which physical device is which) ----

// Feeds a raw-input event into the activity tracker. Called from the host's raw input window.
void RecordInputActivity(unsigned int deviceHandle, bool isKeyboard);

// True if the given mouse/keyboard device handle was used very recently (within the glow window).
bool IsDeviceActive(unsigned int handle);

// Polls connected controllers and remembers which one was last used.
void PollControllerActivity();

// True if the controller at this 1-based XInput slot was used very recently.
bool IsControllerActive(unsigned int xinputSlot);

}
