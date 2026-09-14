#pragma once
#include <WinUser.h>
#include "MessageFilterBase.h"
#include "splitplayloader.h"

namespace SplitPlay
{

class MouseWheelFilter : public MessageFilterBase<SplitPlayMessageFilterIDs::MouseWheelFilterID, WM_MOUSEWHEEL>
{
public:
	static constexpr unsigned int splitPlaySignature = 0x100;

	
	static bool Filter(unsigned int message, unsigned int* lparam, unsigned int* wparam, intptr_t hwnd)
	{
		if ((*wparam & splitPlaySignature) != 0)
		{
			*wparam = (*wparam) & (~splitPlaySignature);
			return true;
		}
		
		return false;
	}

	static const char* Name() { return "Mouse Wheel"; }
	static const char* Description()
	{
		return
			"Filters WM_MOUSEWHEEL messages to only pass those that are synthesized by SplitPlay Input. "
			"This avoids the scroll wheel being sent twice. ";
	}
};

}
