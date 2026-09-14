#include "OpenXinputWrapper.h"
#include <cstdio>

#define OPENXINPUT_XUSER_MAX_COUNT 64
#include "OpenXinput.h"

// OpenXinput requires this function, but it needs mincore.lib which adds a ton of unwanted dependencies,
// so here is a fudge that's (probably?) ok (?)
extern "C" BOOL WINAPI QuirkIsEnabled(ULONG quirk)
{
	return FALSE;
}

namespace OpenXinput
{

DWORD WINAPI SplitPlayOpenXinputGetState(DWORD dwUserIndex, void* pState, bool extended)
{
	return extended ?
		OpenXInputGetStateEx(dwUserIndex, (XINPUT_STATE*)pState) :
		OpenXInputGetState(dwUserIndex, (XINPUT_STATE*)pState);
}

DWORD WINAPI SplitPlayOpenXinputSetState(DWORD dwUserIndex, void* pVibration)
{
	return OpenXInputSetState(dwUserIndex, (XINPUT_VIBRATION*)pVibration);
}

DWORD WINAPI SplitPlayOpenXinputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, void* pCapabilities)
{
	return OpenXInputGetCapabilities(dwUserIndex, dwFlags, (XINPUT_CAPABILITIES*)pCapabilities);
}

}
