#pragma once
#include <WinUser.h>
#include "MessageFilterBase.h"
#include "splitplayloader.h"
#include "HwndSelector.h"


namespace SplitPlay
{

class MouseActivateFilter : public SplitPlay::MessageFilterBase<SplitPlayMessageFilterIDs::MouseActivateFilterID, WM_MOUSEACTIVATE>
{
public:
	static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd)
	{
		*wparam = HwndSelector::GetSelectedHwnd();
		*lparam = 1;
		return true;
	}

	static const char* Name() { return "Mouse Activate"; }
	static const char* Description()
	{
		return
			"WM_MOUSEACTIVATE is sent whenever a mouse clicks on a window. "
			"This filter makes sure the message tells the game it is being focused. ";
	}
};

}
