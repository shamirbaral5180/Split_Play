#pragma once
#include <string>
#include <vector>

namespace SplitPlayHost
{

enum class ControllerApi
{
	XInput,
	OpenXInput,
	DirectInput
};

struct ControllerInfo
{
	// Index exposed to the user, starting at 1 (SplitPlay uses 0 to mean "no controller")
	unsigned int index = 0;

	std::wstring name;

	ControllerApi api = ControllerApi::XInput;

	// Whether SplitPlay needs Dinput -> Xinput redirection for this device
	bool requiresDinputRedirection = false;

	// Whether SplitPlay needs OpenXinput for this device
	bool requiresOpenXinput = false;
};

// Enumerates currently connected controllers (XInput, OpenXInput and DirectInput gamepads/joysticks)
std::vector<ControllerInfo> EnumerateControllers();

// Returns true if at least one controller is connected
bool AnyControllerConnected();

}
