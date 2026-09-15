#include "DisplayUtils.h"
#include "Instance.h"

#include <cwchar>
#include <algorithm>
#include <mutex>
#include <thread>
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

// ---------------- Identify helper: big number overlay on a monitor ----------------
//
// The overlay runs on its own thread with its own message pump, because the main
// GUI loop only pumps messages for its own window and would never paint this one.

namespace
{
	const wchar_t* kFlashClass = L"SplitPlayMonitorFlash";

	std::mutex g_flashMutex;
	std::thread g_flashThread;
	bool g_flashThreadRunning = false;

	// Parameters handed to the overlay thread
	struct FlashParams
	{
		MonitorInfo monitor{};
		int displayNumber = 1;
	};

	LRESULT CALLBACK FlashWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			HDC hdc = BeginPaint(hWnd, &ps);

			RECT rect{};
			GetClientRect(hWnd, &rect);

			// Dim the whole monitor so it is obvious which screen this is
			HBRUSH dim = CreateSolidBrush(RGB(10, 12, 18));
			FillRect(hdc, &rect, dim);
			DeleteObject(dim);

			const auto* textPtr = reinterpret_cast<const std::wstring*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
			const std::wstring text = textPtr != nullptr ? *textPtr : L"";

			// Big centred number/name so the user can read it from across the room
			HFONT font = CreateFontW(-(rect.bottom - rect.top) / 4, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
									 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
									 CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
			HFONT oldFont = (HFONT)SelectObject(hdc, font);

			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(129, 140, 248));
			DrawTextW(hdc, text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			SelectObject(hdc, oldFont);
			DeleteObject(font);

			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_ERASEBKGND:
			return 1; // painted in WM_PAINT
		case WM_TIMER:
			DestroyWindow(hWnd);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	void FlashThreadMain(FlashParams params)
	{
		const auto hinstance = GetModuleHandleW(nullptr);

		static bool registered = false;
		if (!registered)
		{
			WNDCLASSW wc{};
			wc.lpfnWndProc = FlashWndProc;
			wc.hInstance = hinstance;
			wc.hbrBackground = nullptr;
			wc.lpszClassName = kFlashClass;
			wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

			if (!RegisterClassW(&wc))
			{
				std::lock_guard<std::mutex> lock(g_flashMutex);
				g_flashThreadRunning = false;
				return;
			}

			registered = true;
		}

		const std::wstring text = L"Display " + std::to_wstring(params.displayNumber);
		auto* textPtr = new std::wstring(text);

		HWND hwnd = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
			kFlashClass, L"",
			WS_POPUP,
			params.monitor.x, params.monitor.y, params.monitor.width, params.monitor.height,
			nullptr, nullptr, hinstance, nullptr);

		if (hwnd == nullptr)
		{
			delete textPtr;

			std::lock_guard<std::mutex> lock(g_flashMutex);
			g_flashThreadRunning = false;
			return;
		}

		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(textPtr));

		ShowWindow(hwnd, SW_SHOWNOACTIVATE);
		SetWindowPos(hwnd, HWND_TOPMOST, params.monitor.x, params.monitor.y,
					 params.monitor.width, params.monitor.height,
					 SWP_NOACTIVATE | SWP_SHOWWINDOW);

		// Close itself after 2 seconds
		SetTimer(hwnd, 1, 2000, nullptr);

		// Own message pump so the overlay paints and the timer fires
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		while (GetMessageW(&msg, nullptr, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		delete textPtr;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);

		std::lock_guard<std::mutex> lock(g_flashMutex);
		g_flashThreadRunning = false;
	}
}

void ClearMonitorFlash()
{
	std::thread toJoin;

	{
		std::lock_guard<std::mutex> lock(g_flashMutex);

		if (!g_flashThread.joinable())
			return;

		if (g_flashThreadRunning)
		{
			// Ask the overlay thread's windows to close, then wait for it below
			const DWORD threadId = GetThreadId(g_flashThread.native_handle());

			EnumThreadWindows(threadId,
							  [](HWND hwnd, LPARAM) -> BOOL
							  {
								  PostMessageW(hwnd, WM_CLOSE, 0, 0);
								  return TRUE;
							  },
							  0);

			PostThreadMessageW(threadId, WM_QUIT, 0, 0);
		}

		// Always take ownership so a finished thread is joined before reuse
		toJoin = std::move(g_flashThread);
	}

	if (toJoin.joinable())
		toJoin.join();
}

void FlashMonitorNumber(const MonitorInfo& monitor, int displayNumber)
{
	// Replace any overlay that is already showing (also joins a finished thread)
	ClearMonitorFlash();

	FlashParams params{};
	params.monitor = monitor;
	params.displayNumber = displayNumber;

	std::lock_guard<std::mutex> lock(g_flashMutex);
	g_flashThreadRunning = true;
	g_flashThread = std::thread(FlashThreadMain, params);
}

}
