#include "framework.h"
#include "include/splitplayutil.h"
#include <cstdio>
#include <set>
#include <mutex>
#include <atomic>

bool isLocked = false;
bool installedHooks = false;
HANDLE threadHandle = nullptr;

// ---- Per-device binding ----
// A Windows low-level mouse/keyboard hook does not report which physical device produced an
// event. Instead, the host feeds us the device handle of every raw input event it sees. We
// remember the recently active devices and swallow input while a bound device is the active one.
//
// The hook fires just before/around the raw input being queued, so we treat a device as
// "currently in use" for a short window rather than requiring an exact match on this event.
static constexpr unsigned long long kActiveWindowMs = 250;

static std::set<unsigned int> boundMice{};
static std::set<unsigned int> boundKeyboards{};
static std::mutex boundMutex{};

static std::atomic<unsigned int> lastMouseDevice{ 0 };
static std::atomic<unsigned int> lastKeyboardDevice{ 0 };
static std::atomic<unsigned long long> lastMouseTick{ 0 };
static std::atomic<unsigned long long> lastKeyboardTick{ 0 };

static bool IsMouseBound()
{
	std::lock_guard<std::mutex> lock(boundMutex);
	if (boundMice.empty())
		return false;

	const auto handle = lastMouseDevice.load();
	if (handle == 0 || boundMice.count(handle) == 0)
		return false;

	return (GetTickCount64() - lastMouseTick.load()) < kActiveWindowMs;
}

static bool IsKeyboardBound()
{
	std::lock_guard<std::mutex> lock(boundMutex);
	if (boundKeyboards.empty())
		return false;

	const auto handle = lastKeyboardDevice.load();
	if (handle == 0 || boundKeyboards.count(handle) == 0)
		return false;

	return (GetTickCount64() - lastKeyboardTick.load()) < kActiveWindowMs;
}

LRESULT CALLBACK LowLevelMouseProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if (nCode < 0)
		return CallNextHookEx(nullptr, nCode, wParam, lParam);

	// Full lock blocks everything, otherwise only swallow bound devices
	if (isLocked)
		return 1;

	if (IsMouseBound())
		return 1;

	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if (nCode < 0)
		return CallNextHookEx(nullptr, nCode, wParam, lParam);

	// if (!isLocked || ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_END)
	if (!isLocked && !IsKeyboardBound())
	{
		return CallNextHookEx(nullptr, nCode, wParam, lParam);
	}
	else
	{
		auto p = (KBDLLHOOKSTRUCT*)lParam;
		p->vkCode = 0;
		return CallNextHookEx(nullptr, nCode, wParam, lParam);
	}
}

static void EnsureHooksInstalled()
{
	if (installedHooks)
		return;

	installedHooks = true;
	SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(0), 0);
	SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(0), 0);
}

DWORD WINAPI LoopThread(LPVOID lpParameter)
{
	while (true)
	{
		if (isLocked)
		{
			SetForegroundWindow(GetDesktopWindow());
			RECT rect{ 0,0,0,0 };
			ClipCursor(&rect);
			// ShowCursor(FALSE);
			// SetCursor(NULL);
		}
		else
		{
			ClipCursor(NULL);
		}

		//TODO: probably not the best idea
		Sleep(5);
	}

	return 0;
}

extern "C" __declspec(dllexport) unsigned int LockInput(bool lock)
{
	isLocked = lock;

	if (threadHandle == nullptr)
	{
		threadHandle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)LoopThread,
									GetModuleHandle(0), CREATE_SUSPENDED, 0);

		// if (threadHandle != nullptr)
		// 	CloseHandle(threadHandle);
	}
	
	if (lock)
	{
		EnsureHooksInstalled();
	}

	if (threadHandle != nullptr)
	{
		if (lock)
			ResumeThread(threadHandle);
		else
		{
			ClipCursor(NULL);
			SuspendThread(threadHandle);
		}
	}

	return (threadHandle != nullptr) ? GetThreadId(threadHandle) : 0;
}

// Called by the host whenever a real mouse/keyboard event arrives, so binding can tell
// which physical device produced an event.
extern "C" __declspec(dllexport) void NotifyActiveInputDevice(unsigned int deviceHandle, bool isKeyboard)
{
	const auto now = GetTickCount64();

	if (isKeyboard)
	{
		lastKeyboardDevice.store(deviceHandle);
		lastKeyboardTick.store(now);
	}
	else
	{
		lastMouseDevice.store(deviceHandle);
		lastMouseTick.store(now);
	}
}

extern "C" __declspec(dllexport) void BindInputDevice(unsigned int deviceHandle, bool isKeyboard)
{
	{
		std::lock_guard<std::mutex> lock(boundMutex);

		if (isKeyboard)
			boundKeyboards.insert(deviceHandle);
		else
			boundMice.insert(deviceHandle);
	}

	EnsureHooksInstalled();
}

extern "C" __declspec(dllexport) void UnbindAllInputDevices()
{
	std::lock_guard<std::mutex> lock(boundMutex);
	boundMice.clear();
	boundKeyboards.clear();
}
