#include "SetWindowPosHook.h"
#include <imgui.h>
#include <windows.h>
#include "HwndSelector.h"

namespace SplitPlay
{

int SetWindowPosHook::width = 0;
int SetWindowPosHook::height = 0;
int SetWindowPosHook::posx = 0;
int SetWindowPosHook::posy = 0;
bool SetWindowPosHook::lockWindow = true;

static bool HasValidTarget()
{
	// width/height == 0 means "not configured yet"
	return SetWindowPosHook::lockWindow && SetWindowPosHook::width > 0 && SetWindowPosHook::height > 0;
}

void SetWindowPosHook::ApplyToMainWindow()
{
	if (!HasValidTarget())
		return;

	const auto hwnd = (HWND)HwndSelector::GetSelectedHwnd();
	if (hwnd == nullptr || !IsWindow(hwnd))
		return;

	// Move it now, even if the app never calls SetWindowPos itself
	::SetWindowPos(hwnd, nullptr, posx, posy, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL WINAPI Hook_SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	if (!HasValidTarget())
		return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

	return SetWindowPos(hWnd, hWndInsertAfter,
						SetWindowPosHook::posx, SetWindowPosHook::posy,
						SetWindowPosHook::width, SetWindowPosHook::height, uFlags);
}

BOOL WINAPI Hook_MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	if (!HasValidTarget())
		return MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);

	return MoveWindow(hWnd, SetWindowPosHook::posx, SetWindowPosHook::posy,
					  SetWindowPosHook::width, SetWindowPosHook::height, bRepaint);
}

BOOL WINAPI Hook_SetWindowPlacement(HWND hWnd, const WINDOWPLACEMENT* lpwndpl)
{
	if (!HasValidTarget() || lpwndpl == nullptr)
		return SetWindowPlacement(hWnd, lpwndpl);

	// Force the normal position to the target display and stop the app restoring its own rect
	WINDOWPLACEMENT placement = *lpwndpl;
	placement.showCmd = SW_SHOWNORMAL;
	placement.rcNormalPosition.left = SetWindowPosHook::posx;
	placement.rcNormalPosition.top = SetWindowPosHook::posy;
	placement.rcNormalPosition.right = SetWindowPosHook::posx + SetWindowPosHook::width;
	placement.rcNormalPosition.bottom = SetWindowPosHook::posy + SetWindowPosHook::height;

	return SetWindowPlacement(hWnd, &placement);
}

void SetWindowPosHook::ShowGuiStatus()
{
	bool lock = lockWindow;
	ImGui::Checkbox("Lock window to this position/size", &lock);
	lockWindow = lock;

	int pos[2] = { posx, posy };
	ImGui::SliderInt2("Position", &pos[0], -5000, 5000);
	posx = pos[0];
	posy = pos[1];
	
	int size[2] = { width, height };
	ImGui::SliderInt2("Size", &size[0], 0, 5000);
	width = size[0];
	height = size[1];
}

void SetWindowPosHook::InstallImpl()
{
	hookInfo = std::get<1>(InstallNamedHook(L"user32", "SetWindowPos", Hook_SetWindowPos));
	hookInfoMoveWindow = std::get<1>(InstallNamedHook(L"user32", "MoveWindow", Hook_MoveWindow));
	hookInfoSetWindowPlacement = std::get<1>(InstallNamedHook(L"user32", "SetWindowPlacement", Hook_SetWindowPlacement));
}

void SetWindowPosHook::UninstallImpl()
{
	UninstallHook(&hookInfo);
	UninstallHook(&hookInfoMoveWindow);
	UninstallHook(&hookInfoSetWindowPlacement);
}

}
