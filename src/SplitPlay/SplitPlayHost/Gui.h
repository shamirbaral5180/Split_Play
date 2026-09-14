#pragma once
#include <windows.h>

namespace SplitPlayHost
{

extern HWND splitPlayHostHwnd;

void RenderImgui();

int ShowGuiImpl();

void OnInputLockChange(bool locked);

}