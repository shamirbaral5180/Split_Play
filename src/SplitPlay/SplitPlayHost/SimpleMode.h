#pragma once
#include <string>
#include <vector>
#include "DisplayUtils.h"
#include "ControllerUtils.h"
#include "DeviceUtils.h"

namespace SplitPlayHost
{

// What is assigned to the game instance
struct Assignment
{
	bool enabled = false;
	unsigned int index = 0;      // controller index (1-based) or device handle
	std::wstring label;          // human readable
};

// State for the simplified "just play on my second screen" flow
struct SimpleModeState
{
	// Game source
	bool launchNewInstance = true;
	std::wstring gameFilepath;
	unsigned long runningPid = 0;
	std::wstring runningProcessName;

	// Discovered games / processes
	std::vector<RunningProcessInfo> runningGames;
	std::string gameSearch;

	// Target display
	int selectedMonitorIndex = 0;
	bool moveWindowToDisplay = true;

	// Devices: which one is selected, and whether it is enabled for the game
	int selectedControllerIndex = 0;
	std::vector<bool> controllerEnabled;   // parallel to controllers

	int selectedMouseIndex = 0;
	std::vector<bool> mouseEnabled;        // parallel to mice

	int selectedKeyboardIndex = 0;
	std::vector<bool> keyboardEnabled;     // parallel to keyboards

	// Options
	bool showFakeCursor = true;
	bool freezeInputUntilStart = false;
	bool lockRealInput = false;

	// Runtime status
	bool running = false;
	bool hasInjected = false;
	unsigned int instanceHandle = 0;
	std::string statusMessage = "";

	// Window locking: remember what we forced so we can re-apply if the app moves itself
	bool windowLockEnabled = false;
	HWND targetHwnd = nullptr;
	unsigned long targetPid = 0;
	int lockX = 0;
	int lockY = 0;
	int lockWidth = 0;
	int lockHeight = 0;

	std::vector<MonitorInfo> monitors;
	std::vector<ControllerInfo> controllers;
	std::vector<DeviceInfo> mice;
	std::vector<DeviceInfo> keyboards;
};

SimpleModeState& GetSimpleModeState();

void RefreshSimpleModeDevices();
void RefreshSimpleModeProcesses();
void RenderSimpleMode();

}
