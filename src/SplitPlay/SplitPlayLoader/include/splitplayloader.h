#pragma once
#include <cstdint>

// 0 means invalid handle
using SplitPlayInstanceHandle = unsigned int;

enum SplitPlayHookIDs : unsigned int
{
	RegisterRawInputHookID = 0,
	GetRawInputDataHookID,
	MessageFilterHookID,
	GetCursorPosHookID,
	SetCursorPosHookID,
	GetKeyStateHookID,
	GetAsyncKeyStateHookID,
	GetKeyboardStateHookID,
	CursorVisibilityStateHookID,
	ClipCursorHookID,
	FocusHooksHookID,
	RenameHandlesHookID,
	XinputHookID,
	DinputOrderHookID,
	SetWindowPosHookID,
	BlockRawInputHookID,
	FindWindowHookID,
	CreateSingleHIDHookID,
	WindowStyleHookID
};

enum SplitPlayMessageFilterIDs : unsigned int
{
	RawInputFilterID = 0,
	MouseMoveFilterID,
	MouseActivateFilterID,
	WindowActivateFilterID,
	WindowActivateAppFilterID,
	MouseWheelFilterID,
	MouseButtonFilterID,
	KeyboardButtonFilterID
};

extern "C" __declspec(dllexport) SplitPlayInstanceHandle RemoteLoadLibraryInjectRuntime(unsigned long pid, const wchar_t* dllFolderPath);

extern "C" __declspec(dllexport) SplitPlayInstanceHandle EasyHookInjectRuntime(unsigned long pid, const wchar_t* dllFolderPath);
extern "C" __declspec(dllexport) SplitPlayInstanceHandle EasyHookStealthInjectRuntime(unsigned long pid, const wchar_t* dllFolderPath);

extern "C" __declspec(dllexport) SplitPlayInstanceHandle EasyHookInjectStartup(
	const wchar_t* exePath,
	const wchar_t* commandLine,
	unsigned long processCreationFlags,
	const wchar_t* dllFolderPath,
	unsigned long* outPid,
	void* environment = nullptr);

extern "C" __declspec(dllexport) void InstallHook(SplitPlayInstanceHandle instanceHandle, SplitPlayHookIDs hookID);
extern "C" __declspec(dllexport) void UninstallHook(SplitPlayInstanceHandle instanceHandle, SplitPlayHookIDs hookID);

extern "C" __declspec(dllexport) void EnableMessageFilter(SplitPlayInstanceHandle instanceHandle, SplitPlayMessageFilterIDs filterID);
extern "C" __declspec(dllexport) void DisableMessageFilter(SplitPlayInstanceHandle instanceHandle, SplitPlayMessageFilterIDs filterID);

extern "C" __declspec(dllexport) void EnableMessageBlock(SplitPlayInstanceHandle instanceHandle, unsigned int messageID);
extern "C" __declspec(dllexport) void DisableMessageBlock(SplitPlayInstanceHandle instanceHandle, unsigned int messageID);

extern "C" __declspec(dllexport) void WakeUpProcess(SplitPlayInstanceHandle instanceHandle);

extern "C" __declspec(dllexport) void UpdateMainWindowHandle(SplitPlayInstanceHandle instanceHandle, uint64_t hwnd = 0);

extern "C" __declspec(dllexport) void SetupState(SplitPlayInstanceHandle instanceHandle, int instanceIndex);

extern "C" __declspec(dllexport) void SetupMessagesToSend(SplitPlayInstanceHandle instanceHandle,
														  bool sendMouseWheelMessages = true, bool sendMouseButtonMessages = true, bool sendMouseMoveMessages = true, bool sendKeyboardPressMessages = true);

extern "C" __declspec(dllexport) void StartFocusMessageLoop(SplitPlayInstanceHandle instanceHandle, int milliseconds = 5,
															bool wm_activate = true, bool wm_activateapp = true, bool wm_ncactivate = true, bool wm_setfocus = true, bool wm_mouseactivate = true);

extern "C" __declspec(dllexport) void StopFocusMessageLoop(SplitPlayInstanceHandle instanceHandle);

extern "C" __declspec(dllexport) void SetDrawFakeCursor(SplitPlayInstanceHandle instanceHandle, bool enable);

extern "C" __declspec(dllexport) void SetExternalFreezeFakeInput(SplitPlayInstanceHandle instanceHandle, bool enableFreeze);

extern "C" __declspec(dllexport) void AddSelectedMouseHandle(SplitPlayInstanceHandle instanceHandle, unsigned int mouseHandle);
extern "C" __declspec(dllexport) void AddSelectedKeyboardHandle(SplitPlayInstanceHandle instanceHandle, unsigned int keyboardHandle);

extern "C" __declspec(dllexport) void SetControllerIndex(SplitPlayInstanceHandle instanceHandle, unsigned int controllerIndex, unsigned int controllerIndex2 = 0, unsigned int controllerIndex3 = 0, unsigned int controllerIndex4 = 0);

// This MUST be called before calling InstallHook on the Xinput hook
extern "C" __declspec(dllexport) void SetUseDinputRedirection(SplitPlayInstanceHandle instanceHandle, bool useRedirection);

extern "C" __declspec(dllexport) void SetUseOpenXinput(SplitPlayInstanceHandle instanceHandle, bool useOpenXinput);

// Both of these functions require RenameHandlesHookHookID hook
extern "C" __declspec(dllexport) void AddHandleToRename(SplitPlayInstanceHandle instanceHandle, const wchar_t* name);

extern "C" __declspec(dllexport) void AddNamedPipeToRename(SplitPlayInstanceHandle instanceHandle, const wchar_t* name);

extern "C" __declspec(dllexport) void SetDinputDeviceGUID(SplitPlayInstanceHandle instanceHandle, 
	unsigned long  Data1,
	unsigned short Data2,
	unsigned short Data3,
	unsigned char  Data4a,
	unsigned char  Data4b,
	unsigned char  Data4c,
	unsigned char  Data4d,
	unsigned char  Data4e,
	unsigned char  Data4f,
	unsigned char  Data4g,
	unsigned char  Data4h);

// This MUST be called before calling InstallHook on the Dinput order hook
extern "C" __declspec(dllexport) void DinputHookAlsoHooksGetDeviceState(SplitPlayInstanceHandle instanceHandle, bool enable);

extern "C" __declspec(dllexport) void SetSetWindowPosSettings(SplitPlayInstanceHandle instanceHandle, int posx, int posy, int width, int height);

extern "C" __declspec(dllexport) void SetCreateSingleHIDName(SplitPlayInstanceHandle instanceHandle, const wchar_t* name);

extern "C" __declspec(dllexport) void SetCursorClipOptions(SplitPlayInstanceHandle instanceHandle, bool useFakeClipCursor);

extern "C" __declspec(dllexport) void AllowFakeCursorOutOfBounds(SplitPlayInstanceHandle instanceHandle, bool allowOutOfBounds, bool extendBounds);

extern "C" __declspec(dllexport) void SetToggleFakeCursorVisibilityShortcut(SplitPlayInstanceHandle instanceHandle, bool enabled, unsigned int vkey);

extern "C" __declspec(dllexport) void SetRawInputBypass(SplitPlayInstanceHandle instanceHandle, bool enabled);

extern "C" __declspec(dllexport) void SetShowCursorWhenImageUpdated(SplitPlayInstanceHandle instanceHandle, bool enabled);