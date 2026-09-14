#include "ControllerUtils.h"

#include <windows.h>
#include <xinput.h>
#include <dinput.h>
#include <algorithm>
#include <utility>

namespace SplitPlayHost
{

typedef DWORD(WINAPI* t_XInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD(WINAPI* t_XInputGetCapabilities)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);

struct XInputApi
{
	HMODULE module = nullptr;
	t_XInputGetState getState = nullptr;
	t_XInputGetCapabilities getCapabilities = nullptr;
};

XInputApi LoadXInputApi()
{
	// Prefer 1_4, fall back to 1_3 and 9_1_0
	const wchar_t* candidates[] = { L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll" };

	for (const auto* name : candidates)
	{
		auto module = LoadLibraryW(name);
		if (module == nullptr)
			continue;

		auto getState = reinterpret_cast<t_XInputGetState>(GetProcAddress(module, "XInputGetState"));
		if (getState == nullptr)
		{
			FreeLibrary(module);
			continue;
		}

		XInputApi api{};
		api.module = module;
		api.getState = getState;
		api.getCapabilities = reinterpret_cast<t_XInputGetCapabilities>(GetProcAddress(module, "XInputGetCapabilities"));
		return api;
	}

	return XInputApi{};
}

void EnumerateXInputControllers(std::vector<ControllerInfo>& controllers, XInputApi& api)
{
	if (api.getState == nullptr)
		return;

	for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex)
	{
		XINPUT_STATE state{};
		if (api.getState(userIndex, &state) != ERROR_SUCCESS)
			continue;

		ControllerInfo info{};
		info.index = userIndex + 1;
		info.api = ControllerApi::XInput;
		info.requiresDinputRedirection = false;
		info.requiresOpenXinput = false;

		XINPUT_CAPABILITIES caps{};
		if (api.getCapabilities != nullptr && api.getCapabilities(userIndex, 0, &caps) == ERROR_SUCCESS)
		{
			info.name = caps.SubType == XINPUT_DEVSUBTYPE_GAMEPAD ? L"Xbox Controller" : L"XInput Controller";
			if (caps.Flags & XINPUT_CAPS_WIRELESS)
				info.name += L" (Wireless)";
		}
		else
			info.name = L"XInput Controller";

		info.name += L" [XInput slot " + std::to_wstring(userIndex + 1) + L"]";

		controllers.push_back(std::move(info));
	}
}

BOOL CALLBACK DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	auto& controllers = *reinterpret_cast<std::vector<ControllerInfo>*>(pvRef);

	// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf page 26:
	//	4 : Joystick
	//	5 : Game Pad
	//	8 : Multi-axis Controller
	const bool isGamepad = (lpddi->wUsage == 4 || lpddi->wUsage == 5 || lpddi->wUsage == 8);

	if (!isGamepad)
		return DIENUM_CONTINUE;

	ControllerInfo info{};
	info.name = lpddi->tszProductName[0] != L'\0' ? lpddi->tszProductName : lpddi->tszInstanceName;
	info.api = ControllerApi::DirectInput;
	info.requiresDinputRedirection = true;
	info.requiresOpenXinput = false;

	controllers.push_back(std::move(info));

	return DIENUM_CONTINUE;
}

void EnumerateDirectInputControllers(std::vector<ControllerInfo>& controllers)
{
	IDirectInput8W* dinput = nullptr;

	const auto createResult = DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
												IID_IDirectInput8W, reinterpret_cast<void**>(&dinput), nullptr);
	if (createResult != DI_OK || dinput == nullptr)
		return;

	dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, &DIEnumDevicesCallback, &controllers, DIEDFL_ATTACHEDONLY);

	dinput->Release();
}

std::vector<ControllerInfo> EnumerateControllers()
{
	std::vector<ControllerInfo> controllers{};

	auto xinputApi = LoadXInputApi();

	// XInput controllers first
	EnumerateXInputControllers(controllers, xinputApi);

	// Then DirectInput-only controllers. XInput devices are also exposed through the
	// DirectInput API (usually as "XBOX 360 For Windows"), so skip those duplicates.
	std::vector<ControllerInfo> dinputControllers{};
	EnumerateDirectInputControllers(dinputControllers);

	auto looksLikeXInputDevice = [](const std::wstring& name)
	{
		const std::wstring upper = [&]
		{
			std::wstring s = name;
			for (auto& c : s) c = towupper(c);
			return s;
		}();

		return upper.find(L"XBOX") != std::wstring::npos ||
			   upper.find(L"XINPUT") != std::wstring::npos;
	};

	for (auto& controller : dinputControllers)
	{
		if (looksLikeXInputDevice(controller.name))
			continue;

		controllers.push_back(std::move(controller));
	}

	// Assign the SplitPlay controller index (1-based) for DirectInput controllers,
	// in the same order the hook enumerates them.
	unsigned int dinputIndex = 1;
	for (auto& controller : controllers)
	{
		if (controller.api == ControllerApi::DirectInput)
		{
			controller.index = dinputIndex++;
			controller.name += L" [DirectInput slot " + std::to_wstring(controller.index) + L"]";
		}
	}

	if (xinputApi.module != nullptr)
		FreeLibrary(xinputApi.module);

	return controllers;
}

bool AnyControllerConnected()
{
	XINPUT_STATE state{};
	auto api = LoadXInputApi();

	bool any = false;
	if (api.getState != nullptr)
	{
		for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex)
		{
			if (api.getState(userIndex, &state) == ERROR_SUCCESS)
			{
				any = true;
				break;
			}
		}
	}

	if (api.module != nullptr)
		FreeLibrary(api.module);

	if (any)
		return true;

	IDirectInput8W* dinput = nullptr;
	const auto createResult = DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
												IID_IDirectInput8W, reinterpret_cast<void**>(&dinput), nullptr);
	if (createResult == DI_OK && dinput != nullptr)
	{
		std::vector<ControllerInfo> controllers{};
		dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, &DIEnumDevicesCallback, &controllers, DIEDFL_ATTACHEDONLY);
		dinput->Release();

		any = !controllers.empty();
	}

	return any;
}

}
