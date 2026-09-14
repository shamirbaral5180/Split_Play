#include "DisplayUtils.h"
#include "Instance.h"

#include <cwchar>
#include <algorithm>
#include <utility>

namespace SplitPlayHost
{

BOOL CALLBACK EnumMonitorCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam)
{
	auto& monitors = *reinterpret_cast<std::vector<MonitorInfo>*>(lParam);

	MONITORINFOEXW monitorInfo{};
	monitorInfo.cbSize = sizeof(MONITORINFOEXW);

	if (!GetMonitorInfoW(hMonitor, &monitorInfo))
		return TRUE;

	MonitorInfo info{};
	info.handle = hMonitor;
	info.deviceName = monitorInfo.szDevice;
	info.x = monitorInfo.rcMonitor.left;
	info.y = monitorInfo.rcMonitor.top;
	info.width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	info.height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
	info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

	// Try and get a friendly name (e.g. SpaceDesk monitor)
	DISPLAY_DEVICEW displayDevice{};
	displayDevice.cb = sizeof(DISPLAY_DEVICEW);

	if (EnumDisplayDevicesW(monitorInfo.szDevice, 0, &displayDevice, 0))
		info.friendlyName = displayDevice.DeviceString;

	if (info.friendlyName.empty())
		info.friendlyName = info.deviceName;

	monitors.push_back(std::move(info));

	return TRUE;
}

std::vector<MonitorInfo> EnumerateMonitors()
{
	std::vector<MonitorInfo> monitors;

	EnumDisplayMonitors(nullptr, nullptr, &EnumMonitorCallback, reinterpret_cast<LPARAM>(&monitors));

	// Put the primary monitor last so a second screen is the first entry
	std::stable_sort(monitors.begin(), monitors.end(),
		[](const MonitorInfo& a, const MonitorInfo& b) { return !a.isPrimary && b.isPrimary; });

	return monitors;
}

MonitorInfo GetMonitorForWindow(HWND hwnd)
{
	MonitorInfo info{};

	if (hwnd == nullptr)
		return info;

	const auto hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);

	MONITORINFOEXW monitorInfo{};
	monitorInfo.cbSize = sizeof(MONITORINFOEXW);

	if (!GetMonitorInfoW(hMonitor, &monitorInfo))
		return info;

	info.handle = hMonitor;
	info.deviceName = monitorInfo.szDevice;
	info.x = monitorInfo.rcMonitor.left;
	info.y = monitorInfo.rcMonitor.top;
	info.width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	info.height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
	info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

	DISPLAY_DEVICEW displayDevice{};
	displayDevice.cb = sizeof(DISPLAY_DEVICEW);

	if (EnumDisplayDevicesW(monitorInfo.szDevice, 0, &displayDevice, 0))
		info.friendlyName = displayDevice.DeviceString;

	if (info.friendlyName.empty())
		info.friendlyName = info.deviceName;

	return info;
}

std::string MonitorLabel(const MonitorInfo& monitor)
{
	std::wstring label = monitor.friendlyName;

	if (monitor.isPrimary)
		label += L" (Main)";

	label += L" - " + std::to_wstring(monitor.width) + L"x" + std::to_wstring(monitor.height);

	if (!monitor.deviceName.empty())
		label += L" [" + monitor.deviceName + L"]";

	return utf8_encode(label);
}

}
