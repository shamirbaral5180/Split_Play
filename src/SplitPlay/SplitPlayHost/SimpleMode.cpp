#include "SimpleMode.h"

namespace SplitPlayHost
{

static void ResizeEnabled(std::vector<bool>& flags, size_t size, bool defaultValue)
{
	flags.resize(size, defaultValue);
}

AppState& GetAppState()
{
	static AppState state{};
	return state;
}

InstanceConfig* GetSelectedInstance()
{
	auto& state = GetAppState();

	for (auto& instance : state.instances)
	{
		if (instance.id == state.selectedInstanceId)
			return &instance;
	}

	return nullptr;
}

int AddInstance()
{
	auto& state = GetAppState();

	InstanceConfig instance{};
	instance.id = state.nextInstanceId++;
	instance.name = L"New app";

	state.instances.push_back(std::move(instance));
	state.selectedInstanceId = state.instances.back().id;

	// Keep the new instance's device lists the right size
	RefreshSimpleModeDevices();

	return state.instances.back().id;
}

void RemoveInstance(int id)
{
	auto& state = GetAppState();

	for (auto it = state.instances.begin(); it != state.instances.end(); ++it)
	{
		if (it->id == id)
		{
			state.instances.erase(it);
			break;
		}
	}

	if (state.selectedInstanceId == id)
		state.selectedInstanceId = state.instances.empty() ? -1 : state.instances.front().id;
}

void RefreshSimpleModeDevices()
{
	auto& state = GetAppState();

	state.monitors = EnumerateMonitors();
	state.controllers = EnumerateControllers();
	state.mice = EnumerateInputDevices(false);
	state.keyboards = EnumerateInputDevices(true);
	state.audioOutputs = EnumerateAudioOutputs();

	for (auto& instance : state.instances)
	{
		if (instance.selectedControllerIndex < 0 || instance.selectedControllerIndex >= (int)state.controllers.size())
			instance.selectedControllerIndex = 0;

		if (instance.selectedMouseIndex < 0 || instance.selectedMouseIndex >= (int)state.mice.size())
			instance.selectedMouseIndex = 0;

		if (instance.selectedKeyboardIndex < 0 || instance.selectedKeyboardIndex >= (int)state.keyboards.size())
			instance.selectedKeyboardIndex = 0;

		// Monitors: keep the previous choice, default the first monitor ON when nothing is set
		const size_t monitorCount = state.monitors.size();
		instance.monitorEnabled.resize(monitorCount, false);
		{
			bool anyMonitor = false;
			for (bool m : instance.monitorEnabled) anyMonitor = anyMonitor || m;
			if (!anyMonitor && monitorCount > 0)
				instance.monitorEnabled[0] = true;
		}

		ResizeEnabled(instance.controllerEnabled, state.controllers.size(), false);
		ResizeEnabled(instance.mouseEnabled, state.mice.size(), false);
		ResizeEnabled(instance.keyboardEnabled, state.keyboards.size(), false);

		// Audio outputs default to none selected (routeAudio stays off until chosen)
		instance.audioEnabled.resize(state.audioOutputs.size(), false);
	}
}

void RefreshSimpleModeProcesses()
{
	auto& state = GetAppState();
	state.runningGames = EnumerateWindowedProcesses();
}

bool IsMouseHandleAssigned(unsigned int handle)
{
	auto& state = GetAppState();

	for (const auto& instance : state.instances)
	{
		for (int i = 0; i < (int)instance.mouseEnabled.size() && i < (int)state.mice.size(); ++i)
		{
			if (instance.mouseEnabled[i] && state.mice[i].handle == handle)
				return true;
		}
	}

	return false;
}

bool IsKeyboardHandleAssigned(unsigned int handle)
{
	auto& state = GetAppState();

	for (const auto& instance : state.instances)
	{
		for (int i = 0; i < (int)instance.keyboardEnabled.size() && i < (int)state.keyboards.size(); ++i)
		{
			if (instance.keyboardEnabled[i] && state.keyboards[i].handle == handle)
				return true;
		}
	}

	return false;
}

bool IsControllerIndexAssigned(unsigned int controllerIndex)
{
	auto& state = GetAppState();

	for (const auto& instance : state.instances)
	{
		for (int i = 0; i < (int)instance.controllerEnabled.size() && i < (int)state.controllers.size(); ++i)
		{
			if (instance.controllerEnabled[i] && state.controllers[i].index == controllerIndex)
				return true;
		}
	}

	return false;
}

void AssignController(int instanceId, int index, bool enabled)
{
	auto& state = GetAppState();

	// Turn the controller off for every instance first (one controller, one app)
	for (auto& instance : state.instances)
	{
		if (index >= 0 && index < (int)instance.controllerEnabled.size())
			instance.controllerEnabled[index] = false;
	}

	for (auto& instance : state.instances)
	{
		if (instance.id == instanceId)
		{
			if (enabled && index >= 0 && index < (int)instance.controllerEnabled.size())
			{
				instance.selectedControllerIndex = index;
				instance.controllerEnabled[index] = true;
			}
			break;
		}
	}
}

void AssignMouse(int instanceId, int index, bool enabled)
{
	auto& state = GetAppState();

	for (auto& instance : state.instances)
	{
		if (index >= 0 && index < (int)instance.mouseEnabled.size())
			instance.mouseEnabled[index] = false;
	}

	for (auto& instance : state.instances)
	{
		if (instance.id == instanceId)
		{
			if (enabled && index >= 0 && index < (int)instance.mouseEnabled.size())
			{
				instance.selectedMouseIndex = index;
				instance.mouseEnabled[index] = true;
			}
			break;
		}
	}
}

void AssignKeyboard(int instanceId, int index, bool enabled)
{
	auto& state = GetAppState();

	for (auto& instance : state.instances)
	{
		if (index >= 0 && index < (int)instance.keyboardEnabled.size())
			instance.keyboardEnabled[index] = false;
	}

	for (auto& instance : state.instances)
	{
		if (instance.id == instanceId)
		{
			if (enabled && index >= 0 && index < (int)instance.keyboardEnabled.size())
			{
				instance.selectedKeyboardIndex = index;
				instance.keyboardEnabled[index] = true;
			}
			break;
		}
	}
}

void ToggleMonitor(int instanceId, int index)
{
	auto& state = GetAppState();

	for (auto& instance : state.instances)
	{
		if (instance.id != instanceId)
			continue;

		if (index < 0 || index >= (int)instance.monitorEnabled.size())
			break;

		// Never allow the app to have no monitor: clicking the last one keeps it on
		const bool wasOn = instance.monitorEnabled[index];

		if (wasOn && CountAssignedMonitors(instance) <= 1)
			break;

		instance.monitorEnabled[index] = !wasOn;
		break;
	}
}

bool IsMonitorUsedByOther(int instanceId, int index)
{
	auto& state = GetAppState();

	for (const auto& instance : state.instances)
	{
		if (instance.id == instanceId)
			continue;

		if (index >= 0 && index < (int)instance.monitorEnabled.size() && instance.monitorEnabled[index])
			return true;
	}

	return false;
}

int CountAssignedMonitors(const InstanceConfig& cfg)
{
	int count = 0;

	for (bool m : cfg.monitorEnabled)
		count += m ? 1 : 0;

	return count;
}

void ToggleAudioOutput(int instanceId, int index)
{
	auto& state = GetAppState();

	for (auto& instance : state.instances)
	{
		if (instance.id != instanceId)
			continue;

		if (index < 0 || index >= (int)instance.audioEnabled.size())
			break;

		instance.audioEnabled[index] = !instance.audioEnabled[index];

		// routeAudio reflects whether any output is selected
		bool any = false;
		for (bool a : instance.audioEnabled) any = any || a;
		instance.routeAudio = any;
		break;
	}
}

}
