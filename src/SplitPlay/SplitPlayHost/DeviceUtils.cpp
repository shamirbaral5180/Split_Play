#include "DeviceUtils.h"
#include "Instance.h"
#include <hidusage.h>
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

static std::atomic<unsigned int> g_LastActiveMouse{ 0 };
static std::atomic<unsigned int> g_LastActiveKeyboard{ 0 };
static std::atomic<unsigned long long> g_LastMouseTick{ 0 };
static std::atomic<unsigned long long> g_LastKeyboardTick{ 0 };

static HANDLE g_MonitorThread = nullptr;
static HANDLE g_MonitorStopEvent = nullptr;
static HWND g_MonitorHwnd = nullptr;

LRESULT CALLBACK InputMonitorWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INPUT)
	{
		RAWINPUT rawInput{};
		UINT size = sizeof(rawInput);

		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &rawInput, &size,
							sizeof(RAWINPUTHEADER)) == size)
		{
			const auto handle = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(rawInput.header.hDevice));
			const auto tick = GetTickCount64();

			if (rawInput.header.dwType == RIM_TYPEMOUSE)
			{
				g_LastActiveMouse.store(handle);
				g_LastMouseTick.store(tick);
			}
			else if (rawInput.header.dwType == RIM_TYPEKEYBOARD)
			{
				g_LastActiveKeyboard.store(handle);
				g_LastKeyboardTick.store(tick);
			}
		}

		DefWindowProcW(hWnd, msg, wParam, lParam);
		return 0;
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

DWORD WINAPI InputMonitorThread(LPVOID)
{
	const auto hInstance = GetModuleHandleW(nullptr);

	WNDCLASSW wc{};
	wc.lpfnWndProc = InputMonitorWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"SplitPlayInputMonitor";

	RegisterClassW(&wc);

	g_MonitorHwnd = CreateWindowExW(0, wc.lpszClassName, L"SplitPlay Input Monitor", 0, 0, 0, 0, 0,
									HWND_MESSAGE, nullptr, hInstance, nullptr);

	if (g_MonitorHwnd != nullptr)
	{
		RAWINPUTDEVICE devices[2]{};
		devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		devices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		devices[0].dwFlags = RIDEV_INPUTSINK;
		devices[0].hwndTarget = g_MonitorHwnd;

		devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
		devices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
		devices[1].dwFlags = RIDEV_INPUTSINK;
		devices[1].hwndTarget = g_MonitorHwnd;

		RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
	}

	MSG msg{};
	while (true)
	{
		const DWORD wait = MsgWaitForMultipleObjects(1, &g_MonitorStopEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (wait == WAIT_OBJECT_0)
			break;

		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	if (g_MonitorHwnd != nullptr)
	{
		DestroyWindow(g_MonitorHwnd);
		g_MonitorHwnd = nullptr;
	}

	UnregisterClassW(wc.lpszClassName, hInstance);

	return 0;
}

void StartInputActivityMonitor()
{
	if (g_MonitorThread != nullptr)
		return;

	g_MonitorStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	g_MonitorThread = CreateThread(nullptr, 0, InputMonitorThread, nullptr, 0, nullptr);
}

void StopInputActivityMonitor()
{
	if (g_MonitorThread == nullptr)
		return;

	if (g_MonitorStopEvent != nullptr)
		SetEvent(g_MonitorStopEvent);

	WaitForSingleObject(g_MonitorThread, 2000);

	CloseHandle(g_MonitorThread);
	g_MonitorThread = nullptr;

	if (g_MonitorStopEvent != nullptr)
	{
		CloseHandle(g_MonitorStopEvent);
		g_MonitorStopEvent = nullptr;
	}
}

unsigned int GetLastActiveMouse()
{
	return g_LastActiveMouse.load();
}

unsigned int GetLastActiveKeyboard()
{
	return g_LastActiveKeyboard.load();
}

bool IsDeviceActive(unsigned int handle)
{
	constexpr unsigned long long glowMs = 1500;

	const auto now = GetTickCount64();

	if (handle == 0)
		return false;

	if (g_LastActiveMouse.load() == handle && (now - g_LastMouseTick.load()) < glowMs)
		return true;

	if (g_LastActiveKeyboard.load() == handle && (now - g_LastKeyboardTick.load()) < glowMs)
		return true;

	return false;
}

}
