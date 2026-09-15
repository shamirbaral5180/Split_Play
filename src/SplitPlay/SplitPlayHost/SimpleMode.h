#pragma once
#include <string>
#include <vector>
#include "DisplayUtils.h"
#include "ControllerUtils.h"
#include "DeviceUtils.h"

namespace SplitPlayHost
{

// One configured app: everything needed to send it to a display with its own devices.
// A device (mouse/keyboard/controller) can be assigned to only ONE instance at a time.
struct InstanceConfig
{
	int id = 0;                          // stable identity for the UI
	std::wstring name = L"New app";      // shown in the sidebar

	// App source
	bool launchNewInstance = true;
	std::wstring gameFilepath;
	unsigned long runningPid = 0;
	std::wstring runningProcessName;

	// Target display
	int selectedMonitorIndex = 0;
	bool moveWindowToDisplay = true;

	// Devices (parallel to the shared device lists in AppState)
	int selectedControllerIndex = 0;
	std::vector<bool> controllerEnabled;

	int selectedMouseIndex = 0;
	std::vector<bool> mouseEnabled;

	int selectedKeyboardIndex = 0;
	std::vector<bool> keyboardEnabled;

	// Options
	bool showFakeCursor = true;
	bool freezeInputUntilStart = false;
	bool lockRealInput = false;

	// Runtime status
	bool running = false;
	bool hasInjected = false;
	unsigned int instanceHandle = 0;
	std::string statusMessage;

	// Window locking: remember what we forced so we can re-apply if the app moves itself
	bool windowLockEnabled = false;
	HWND targetHwnd = nullptr;
	unsigned long targetPid = 0;
	int lockX = 0;
	int lockY = 0;
	int lockWidth = 0;
	int lockHeight = 0;
};

// Shared application state: discovery lists plus every added instance.
struct AppState
{
	std::vector<InstanceConfig> instances;
	int selectedInstanceId = -1;
	int nextInstanceId = 1;

	// Discovered apps / processes (shared, used when adding an instance)
	std::vector<RunningProcessInfo> runningGames;
	std::string gameSearch;

	// Shared hardware discovery (instances index into these)
	std::vector<MonitorInfo> monitors;
	std::vector<ControllerInfo> controllers;
	std::vector<DeviceInfo> mice;
	std::vector<DeviceInfo> keyboards;
};

AppState& GetAppState();

// Returns the currently selected instance, or nullptr when none is selected.
InstanceConfig* GetSelectedInstance();

// Adds a fresh instance and selects it. Returns its id.
int AddInstance();

// Removes an instance by id (caller should ensure it is not running).
void RemoveInstance(int id);

void RefreshSimpleModeDevices();
void RefreshSimpleModeProcesses();
void RenderSimpleMode();

// True if this mouse/keyboard handle is assigned to any instance.
bool IsMouseHandleAssigned(unsigned int handle);
bool IsKeyboardHandleAssigned(unsigned int handle);
bool IsControllerIndexAssigned(unsigned int controllerIndex);

// Exclusive assignment: turning a device ON for one instance turns it OFF everywhere else.
void AssignController(int instanceId, int index, bool enabled);
void AssignMouse(int instanceId, int index, bool enabled);
void AssignKeyboard(int instanceId, int index, bool enabled);

// Binds the union of all assigned devices from running instances to the input locker.
void RebindAllInputDevices();

// Starts / stops a single instance.
bool StartInstance(int id);
void StopInstance(int id);

}
