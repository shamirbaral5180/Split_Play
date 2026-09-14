#include "SimpleMode.h"

namespace SplitPlayHost
{

static void ResizeEnabled(std::vector<bool>& flags, size_t size, bool defaultValue)
{
	flags.resize(size, defaultValue);
}

SimpleModeState& GetSimpleModeState()
{
	static SimpleModeState state{};
	return state;
}

void RefreshSimpleModeDevices()
{
	auto& state = GetSimpleModeState();

	state.monitors = EnumerateMonitors();
	if (state.selectedMonitorIndex < 0 || state.selectedMonitorIndex >= (int)state.monitors.size())
		state.selectedMonitorIndex = 0;

	state.controllers = EnumerateControllers();
	if (state.selectedControllerIndex < 0 || state.selectedControllerIndex >= (int)state.controllers.size())
		state.selectedControllerIndex = 0;
	ResizeEnabled(state.controllerEnabled, state.controllers.size(), false);
	// Default: enable the first controller if the user hasn't enabled any yet
	if (!state.controllers.empty())
	{
		bool anyEnabled = false;
		for (bool e : state.controllerEnabled) anyEnabled = anyEnabled || e;
		if (!anyEnabled) state.controllerEnabled[0] = true;
	}

	state.mice = EnumerateInputDevices(false);
	if (state.selectedMouseIndex < 0 || state.selectedMouseIndex >= (int)state.mice.size())
		state.selectedMouseIndex = 0;
	ResizeEnabled(state.mouseEnabled, state.mice.size(), false);

	state.keyboards = EnumerateInputDevices(true);
	if (state.selectedKeyboardIndex < 0 || state.selectedKeyboardIndex >= (int)state.keyboards.size())
		state.selectedKeyboardIndex = 0;
	ResizeEnabled(state.keyboardEnabled, state.keyboards.size(), false);
}

void RefreshSimpleModeProcesses()
{
	auto& state = GetSimpleModeState();
	state.runningGames = EnumerateWindowedProcesses();
}

}

