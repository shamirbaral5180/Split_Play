#include "HookManager.h"
#include "RegisterRawInputHook.h"
#include "GetRawInputDataHook.h"
#include "MessageFilterHook.h"
#include "FocusHook.h"
#include "GetCursorPosHook.h"
#include "SetCursorPosHook.h"
#include "GetKeyStateHook.h"
#include "GetAsyncKeyStateHook.h"
#include "GetKeyboardStateHook.h"
#include "CursorVisibilityHook.h"
#include "ClipCursorHook.h"
#include "RenameHandlesHook.h"
#include "XinputHook.h"
#include "DinputOrderHook.h"
#include "SetWindowPosHook.h"
#include "BlockRawInputHook.h"
#include "FindWindowHook.h"
#include "CreateSingleHIDHook.h"
#include "WindowStyleHook.h"

namespace SplitPlay
{

HookManager HookManager::hookManagerInstance{};

HookManager::HookManager()
{
	// Do these in exactly the same order as in SplitPlayHookIDs
	AddHook<RegisterRawInputHook>(SplitPlayHookIDs::RegisterRawInputHookID);
	AddHook<GetRawInputDataHook>(SplitPlayHookIDs::GetRawInputDataHookID);
	AddHook<MessageFilterHook>(SplitPlayHookIDs::MessageFilterHookID);
	AddHook<GetCursorPosHook>(SplitPlayHookIDs::GetCursorPosHookID);
	AddHook<SetCursorPosHook>(SplitPlayHookIDs::SetCursorPosHookID);
	AddHook<GetKeyStateHook>(SplitPlayHookIDs::GetKeyStateHookID);
	AddHook<GetAsyncKeyStateHook>(SplitPlayHookIDs::GetAsyncKeyStateHookID);
	AddHook<GetKeyboardStateHook>(SplitPlayHookIDs::GetKeyboardStateHookID);
	AddHook<CursorVisibilityHook>(SplitPlayHookIDs::CursorVisibilityStateHookID);
	AddHook<ClipCursorHook>(SplitPlayHookIDs::ClipCursorHookID);
	AddHook<FocusHook>(SplitPlayHookIDs::FocusHooksHookID);
	AddHook<RenameHandlesHook>(SplitPlayHookIDs::RenameHandlesHookID);
	AddHook<XinputHook>(SplitPlayHookIDs::XinputHookID);
	AddHook<DinputOrderHook>(SplitPlayHookIDs::DinputOrderHookID);
	AddHook<SetWindowPosHook>(SplitPlayHookIDs::SetWindowPosHookID);
	AddHook<BlockRawInputHook>(SplitPlayHookIDs::BlockRawInputHookID);
	AddHook<FindWindowHook>(SplitPlayHookIDs::FindWindowHookID);
	AddHook<CreateSingleHIDHook>(SplitPlayHookIDs::CreateSingleHIDHookID);
	AddHook<WindowStyleHook>(SplitPlayHookIDs::WindowStyleHookID);
}

void HookManager::InstallHook(SplitPlayHookIDs hookID)
{
	if (hookID < 0 || hookID >= hookManagerInstance.hooks.size())
		std::cerr << "Trying to install hook ID " << hookID << " which is out of range" << std::endl;
	else
	{
		hookManagerInstance.hooks[hookID]->Install();
	}
}

void HookManager::UninstallHook(SplitPlayHookIDs hookID)
{
	if (hookID < 0 || hookID >= hookManagerInstance.hooks.size())
		std::cerr << "Trying to uninstall hook ID " << hookID << " which is out of range" << std::endl;
	else
	{
		hookManagerInstance.hooks[hookID]->Uninstall();
	}
}

bool HookManager::IsInstalled(SplitPlayHookIDs hookID)
{
	if (hookID < 0 || hookID >= hookManagerInstance.hooks.size())
	{
		std::cerr << "Trying to check hook ID " << hookID << " which is out of range" << std::endl;
		return false;
	}
	else
	{
		return hookManagerInstance.hooks[hookID]->IsInstalled();
	}
}

}
