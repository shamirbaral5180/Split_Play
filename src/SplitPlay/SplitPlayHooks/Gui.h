#pragma once
#include <windows.h>
#include <cstdint>

namespace SplitPlay
{

extern unsigned long GuiThreadID;
extern intptr_t ConsoleHwnd;
extern HWND SplitPlayGuiHwnd;

int ShowGuiImpl();

void RenderImgui();

void ToggleWindow();
void SetWindowVisible(bool visible);

void ToggleConsole();
void SetConsoleVisible(bool visible);

}
