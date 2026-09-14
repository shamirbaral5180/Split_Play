#include "Profiles.h"
#include <iostream>
#include <fstream>
#include <Windows.h>

namespace SplitPlayHost
{

std::filesystem::path GetProfilesFolderPath()
{
	static std::filesystem::path folderPath;

	if (static bool init = true; init)
	{
		init = false;

		wchar_t pathchars[MAX_PATH];
		GetModuleFileNameW(NULL, pathchars, MAX_PATH);
		std::wstring exeFolder = pathchars;
		size_t pos = exeFolder.find_last_of(L"\\");
		if (pos != std::string::npos)
			exeFolder = exeFolder.substr(0, pos + 1);

		folderPath = std::filesystem::path(exeFolder);
		folderPath /= "Profiles";

		std::filesystem::create_directory(folderPath);
	}

	return folderPath;
}

inline std::filesystem::path GetFilePath(const std::string& filename)
{
	// return GetProfilesFolderPath() / (filename + ".json");
	return GetProfilesFolderPath() / (filename);
}

void Profile::SaveToFile(const Profile& data, const std::string& name)
{
	std::ofstream file;
	file.open(GetFilePath(name), std::ios::out | std::ios::trunc);

	if (file.fail())
	{
		MessageBoxA(NULL, "Failed to open file", "Error", MB_OK);
	}
	else
	{
		cereal::JSONOutputArchive oarchive(file);
		oarchive(cereal::make_nvp("data", data));
	}

	file.close();
}

void Profile::LoadFromFile(Profile& data, const std::string& name)
{
	std::ifstream file;
	file.open(GetFilePath(name), std::ios::in);

	if (file.fail())
	{
		MessageBoxA(NULL, "Failed to open file", "Error", MB_OK);
	}
	else
	{
		try
		{
			cereal::JSONInputArchive iarchive(file);
			iarchive(cereal::make_nvp("data", data));
		}
		catch (...)
		{
			// Happens when you have a config from an older version that doesn't have all the options. Still works though.
		}
	}

	file.close();
}

bool Profile::DoesProfileFileAlreadyExist(const std::string& name)
{
	return exists(GetFilePath(name));
}

std::vector<std::string> Profile::GetAllProfiles()
{
	std::vector<std::string> profiles{};

	for (const auto& entry : std::filesystem::directory_iterator(GetProfilesFolderPath()))
	{
		profiles.push_back(entry.path().filename().string());
	}

	return profiles;
}

Profile Profile::MakeSecondScreenControllerProfile()
{
	Profile profile{};

	// Hooks needed to keep a non-focused game running and receiving controller input only
	auto setHook = [&profile](SplitPlayHookIDs id, bool enabled)
	{
		for (auto& hook : profile.hooks)
		{
			if (hook.id == (unsigned int)id)
				hook.enabled = enabled;
		}
	};

	auto setFilter = [&profile](SplitPlayMessageFilterIDs id, bool enabled)
	{
		for (auto& filter : profile.messageFilters)
		{
			if (filter.id == (unsigned int)id)
				filter.enabled = enabled;
		}
	};

	// Core hooks for a second-screen controller setup
	setHook(SplitPlayHookIDs::RegisterRawInputHookID, true);
	setHook(SplitPlayHookIDs::GetRawInputDataHookID, true);
	setHook(SplitPlayHookIDs::MessageFilterHookID, true);
	setHook(SplitPlayHookIDs::GetCursorPosHookID, true);
	setHook(SplitPlayHookIDs::SetCursorPosHookID, true);
	setHook(SplitPlayHookIDs::GetKeyStateHookID, true);
	setHook(SplitPlayHookIDs::GetAsyncKeyStateHookID, true);
	setHook(SplitPlayHookIDs::GetKeyboardStateHookID, true);
	setHook(SplitPlayHookIDs::CursorVisibilityStateHookID, true);
	setHook(SplitPlayHookIDs::ClipCursorHookID, true);
	setHook(SplitPlayHookIDs::FocusHooksHookID, true);
	setHook(SplitPlayHookIDs::XinputHookID, true);
	setHook(SplitPlayHookIDs::BlockRawInputHookID, true);
	setHook(SplitPlayHookIDs::SetWindowPosHookID, true);

	// Not needed for a single game on a second screen
	setHook(SplitPlayHookIDs::RenameHandlesHookID, false);
	setHook(SplitPlayHookIDs::DinputOrderHookID, false);
	setHook(SplitPlayHookIDs::FindWindowHookID, false);
	setHook(SplitPlayHookIDs::CreateSingleHIDHookID, false);
	setHook(SplitPlayHookIDs::WindowStyleHookID, false);

	// Filter out the real keyboard/mouse so they stay with Windows
	setFilter(SplitPlayMessageFilterIDs::RawInputFilterID, true);
	setFilter(SplitPlayMessageFilterIDs::MouseMoveFilterID, true);
	setFilter(SplitPlayMessageFilterIDs::MouseActivateFilterID, true);
	setFilter(SplitPlayMessageFilterIDs::WindowActivateFilterID, true);
	setFilter(SplitPlayMessageFilterIDs::WindowActivateAppFilterID, true);
	setFilter(SplitPlayMessageFilterIDs::MouseWheelFilterID, true);
	setFilter(SplitPlayMessageFilterIDs::MouseButtonFilterID, true);
	setFilter(SplitPlayMessageFilterIDs::KeyboardButtonFilterID, true);

	profile.dinputToXinputRedirection = false;
	profile.useOpenXinput = false;
	profile.useFakeClipCursor = true;

	profile.drawFakeMouseCursor = true;
	profile.allowMouseOutOfBounds = false;
	profile.extendMouseBounds = false;
	profile.toggleFakeCursorVisibilityShortcut = false;

	// Don't send any real mouse/keyboard messages to the game
	profile.sendMouseMovementMessages = false;
	profile.sendMouseButtonMessages = false;
	profile.sendMouseWheelMessages = false;
	profile.sendKeyboardButtonMessages = false;

	// Keep the game thinking it is focused so it doesn't pause while we use the desktop
	profile.focusMessageLoop = true;
	profile.focusLoopSendWM_ACTIVATE = true;
	profile.focusLoopSendWM_NCACTIVATE = false;
	profile.focusLoopSendWM_ACTIVATEAPP = true;
	profile.focusLoopSendWM_SETFOCUS = true;
	profile.focusLoopSendWM_MOUSEACTIVATE = true;

	return profile;
}

}