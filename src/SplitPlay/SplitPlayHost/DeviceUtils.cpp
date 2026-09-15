#include "DeviceUtils.h"
#include "Instance.h"
#include <hidusage.h>
#include <xinput.h>
#include <algorithm>
#include <cwchar>
#include <filesystem>
#include <utility>
#include <map>
#include <atomic>

#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>

#pragma comment(lib, "setupapi.lib")

namespace SplitPlayHost
{

static std::wstring ToUpper(std::wstring s)
{
	for (auto& c : s)
		c = towupper(c);
	return s;
}

std::wstring ExtractVidPid(const std::wstring& devicePath)
{
	const auto upper = ToUpper(devicePath);
	const auto vidPos = upper.find(L"VID_");
	const auto pidPos = upper.find(L"PID_");

	if (vidPos == std::wstring::npos || pidPos == std::wstring::npos)
		return L"";

	// VID_ is 4 chars + 4 hex, &PID_ is 4 chars + 4 hex
	const auto vidEnd = vidPos + 8;
	if (pidPos <= vidEnd || pidPos + 8 > upper.size())
		return L"";

	return upper.substr(vidPos, 8) + upper.substr(pidPos, 8);
}

std::vector<HidFriendlyName> EnumerateHidFriendlyNames()
{
	std::vector<HidFriendlyName> names{};

	HDEVINFO deviceInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_HIDCLASS, nullptr, nullptr, DIGCF_PRESENT);
	if (deviceInfo == INVALID_HANDLE_VALUE)
		return names;

	SP_DEVINFO_DATA deviceInfoData{};
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfo, i, &deviceInfoData); ++i)
	{
		wchar_t friendlyName[512]{};
		wchar_t description[512]{};

		if (!SetupDiGetDeviceRegistryPropertyW(deviceInfo, &deviceInfoData, SPDRP_FRIENDLYNAME,
											   nullptr, reinterpret_cast<PBYTE>(friendlyName), sizeof(friendlyName), nullptr))
		{
			SetupDiGetDeviceRegistryPropertyW(deviceInfo, &deviceInfoData, SPDRP_DEVICEDESC,
											  nullptr, reinterpret_cast<PBYTE>(friendlyName), sizeof(friendlyName), nullptr);
		}

		SetupDiGetDeviceRegistryPropertyW(deviceInfo, &deviceInfoData, SPDRP_DEVICEDESC,
										  nullptr, reinterpret_cast<PBYTE>(description), sizeof(description), nullptr);

		if (friendlyName[0] == L'\0')
			continue;

		// Find an instance path containing VID/PID
		HidFriendlyName entry{};
		entry.friendlyName = friendlyName;
		entry.description = description;

		// Try to match by the device instance id
		wchar_t instanceId[1024]{};
		if (CM_Get_Device_IDW(deviceInfoData.DevInst, instanceId, sizeof(instanceId) / sizeof(wchar_t), 0) == CR_SUCCESS)
		{
			entry.vidPid = ExtractVidPid(instanceId);
		}

		if (!entry.vidPid.empty())
			names.push_back(std::move(entry));
	}

	SetupDiDestroyDeviceInfoList(deviceInfo);
	return names;
}

std::wstring LookupDeviceFriendlyName(const std::wstring& devicePath)
{
	static const auto names = EnumerateHidFriendlyNames();

	const auto vidPid = ExtractVidPid(devicePath);
	if (vidPid.empty())
		return L"";

	for (const auto& entry : names)
	{
		if (entry.vidPid == vidPid)
			return entry.friendlyName;
	}

	return L"";
}

// Guesses a device type category for icon selection
std::vector<DeviceInfo> EnumerateInputDevices(bool keyboards)
{
	std::vector<DeviceInfo> devices{};

	UINT numDevices = 0;
	if (GetRawInputDeviceList(nullptr, &numDevices, sizeof(RAWINPUTDEVICELIST)) != 0 || numDevices == 0)
		return devices;

	std::vector<RAWINPUTDEVICELIST> list(numDevices);
	const UINT fetched = GetRawInputDeviceList(list.data(), &numDevices, sizeof(RAWINPUTDEVICELIST));
	if (fetched == (UINT)-1)
		return devices;

	int index = 0;
	for (UINT i = 0; i < numDevices; ++i)
	{
		const auto& device = list[i];

		const bool isKeyboard = device.dwType == RIM_TYPEKEYBOARD;
		const bool isMouse = device.dwType == RIM_TYPEMOUSE;

		if ((keyboards && !isKeyboard) || (!keyboards && !isMouse))
			continue;

		DeviceInfo info{};
		info.isKeyboard = isKeyboard;
		info.handle = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(device.hDevice));

		wchar_t rawPath[512]{};
		UINT size = sizeof(rawPath);
		GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, rawPath, &size);

		const std::wstring devicePath = rawPath;

		// Prefer the real product name from SetupAPI
		std::wstring friendlyName = LookupDeviceFriendlyName(devicePath);

		if (friendlyName.empty())
		{
			// Fall back to the VID/PID token
			const auto vidPid = ExtractVidPid(devicePath);
			if (!vidPid.empty())
				friendlyName = vidPid;
			else
				friendlyName = isKeyboard ? L"Keyboard" : L"Mouse";
		}

		info.name = std::move(friendlyName);

		// Tag with a stable index so the UI can tell identical devices apart
		++index;
		info.name += (isKeyboard ? L"  (Keyboard " : L"  (Mouse ") + std::to_wstring(index) + L")";

		devices.push_back(std::move(info));
	}

	return devices;
}

struct EnumWindowsState
{
	std::vector<RunningProcessInfo> processes;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	auto& state = *reinterpret_cast<EnumWindowsState*>(lParam);

	if (!IsWindowVisible(hwnd))
		return TRUE;

	const int textLength = GetWindowTextLengthW(hwnd);
	if (textLength == 0)
		return TRUE;

	wchar_t title[512]{};
	GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));

	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == 0)
		return TRUE;

	const auto ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (ph == nullptr)
		return TRUE;

	wchar_t pathBuffer[MAX_PATH]{};
	DWORD buffSize = sizeof(pathBuffer) / sizeof(wchar_t);
	QueryFullProcessImageNameW(ph, 0, pathBuffer, &buffSize);
	CloseHandle(ph);

	std::wstring processName = std::filesystem::path(pathBuffer).filename().wstring();
	if (processName.empty())
		return TRUE;

	// Skip our own window and obvious shell/system windows
	if (processName == L"SplitPlay.exe" || processName == L"explorer.exe" ||
		processName == L"SearchApp.exe" || processName == L"TextInputHost.exe")
		return TRUE;

	// Avoid duplicate pids (keep the first window title we saw)
	for (auto& existing : state.processes)
	{
		if (existing.pid == pid)
			return TRUE;
	}

	RunningProcessInfo info{};
	info.pid = pid;
	info.name = processName;
	info.windowTitle = title;
	info.hasWindow = true;

	state.processes.push_back(std::move(info));

	return TRUE;
}

std::vector<RunningProcessInfo> EnumerateWindowedProcesses()
{
	EnumWindowsState state{};
	EnumWindows(&EnumWindowsProc, reinterpret_cast<LPARAM>(&state));

	std::sort(state.processes.begin(), state.processes.end(),
		[](const RunningProcessInfo& a, const RunningProcessInfo& b) { return a.name < b.name; });

	return state.processes;
}

// ---------------- Live input activity tracking ----------------
//
// Mice and keyboards arrive as raw input (fed in by the host's raw input window),
// while controllers are polled through XInput. We only need "what was used recently"
// so the UI can light up the matching row.

static std::atomic<unsigned int> g_LastActiveMouse{ 0 };
static std::atomic<unsigned int> g_LastActiveKeyboard{ 0 };
static std::atomic<unsigned long long> g_LastMouseTick{ 0 };
static std::atomic<unsigned long long> g_LastKeyboardTick{ 0 };

static std::atomic<unsigned int> g_LastActiveController{ 0 }; // 1-based XInput slot
static std::atomic<unsigned long long> g_LastControllerTick{ 0 };

static constexpr unsigned long long kGlowMs = 1500;

void RecordInputActivity(unsigned int deviceHandle, bool isKeyboard)
{
	const auto tick = GetTickCount64();

	if (isKeyboard)
	{
		g_LastActiveKeyboard.store(deviceHandle);
		g_LastKeyboardTick.store(tick);
	}
	else
	{
		g_LastActiveMouse.store(deviceHandle);
		g_LastMouseTick.store(tick);
	}
}

bool IsDeviceActive(unsigned int handle)
{
	if (handle == 0)
		return false;

	const auto now = GetTickCount64();

	if (g_LastActiveMouse.load() == handle && (now - g_LastMouseTick.load()) < kGlowMs)
		return true;

	if (g_LastActiveKeyboard.load() == handle && (now - g_LastKeyboardTick.load()) < kGlowMs)
		return true;

	return false;
}

void PollControllerActivity()
{
	// Load XInput dynamically (xinput1_4 isn't present on older Windows)
	typedef DWORD(WINAPI* t_XInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState);

	static t_XInputGetState getState = []
	{
		const wchar_t* candidates[] = { L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll" };

		for (const auto* name : candidates)
		{
			if (auto module = LoadLibraryW(name))
			{
				if (auto fn = reinterpret_cast<t_XInputGetState>(GetProcAddress(module, "XInputGetState")))
					return fn;

				FreeLibrary(module);
			}
		}

		return static_cast<t_XInputGetState>(nullptr);
	}();

	if (getState == nullptr)
		return;

	static XINPUT_GAMEPAD previous[4]{};

	for (DWORD slot = 0; slot < 4; ++slot)
	{
		XINPUT_STATE current{};
		if (getState(slot, &current) != ERROR_SUCCESS)
			continue;

		// Compare against the previous sample to detect movement/button presses.
		const auto& now = current.Gamepad;
		const auto& before = previous[slot];

		const bool changed =
			now.wButtons != before.wButtons ||
			now.bLeftTrigger != before.bLeftTrigger ||
			now.bRightTrigger != before.bRightTrigger ||
			now.sThumbLX != before.sThumbLX ||
			now.sThumbLY != before.sThumbLY ||
			now.sThumbRX != before.sThumbRX ||
			now.sThumbRY != before.sThumbRY;

		previous[slot] = now;

		if (changed)
		{
			g_LastActiveController.store(slot + 1);
			g_LastControllerTick.store(GetTickCount64());
		}
	}
}

bool IsControllerActive(unsigned int xinputSlot)
{
	if (xinputSlot == 0)
		return false;

	const auto now = GetTickCount64();
	return g_LastActiveController.load() == xinputSlot && (now - g_LastControllerTick.load()) < kGlowMs;
}

}
