#pragma once
#include <filesystem>
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/complex.hpp>
#include "splitplayloader.h"
#include "MessageList.h"

namespace SplitPlayHost
{

struct ProfileOption
{
	std::string name;
	std::string uiLabel;
	bool enabled = false;
	unsigned int id;
	
	ProfileOption() = default;
	
	template<class T = unsigned int>
	ProfileOption(std::string _name, bool _enabled, T _id = 0) : name(std::move(_name)), uiLabel(name), enabled(_enabled), id((unsigned int)_id) {}

	template<class T = unsigned int>
	ProfileOption(std::string _name, bool _enabled, std::string _uiLabel, T _id = 0) : name(std::move(_name)), uiLabel(std::move(_uiLabel)),
			enabled(_enabled), id((unsigned int)_id) {}

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp("name", name),
			cereal::make_nvp("enabled", enabled)
		);
	}
};

struct Profile
{
	std::vector<ProfileOption> hooks
	{
		{ "Register Raw Input", true, "Register Raw Input", SplitPlayHookIDs::RegisterRawInputHookID },
		{ "Get Raw Input Data", true, "Get Raw Input Data", SplitPlayHookIDs::GetRawInputDataHookID },
		{ "Message Filter", true, "Message Filter", SplitPlayHookIDs::MessageFilterHookID },
		{ "Get Cursor Position", true, "Get Cursor Position", SplitPlayHookIDs::GetCursorPosHookID },
		{ "Set Cursor Position", true, "Set Cursor Position", SplitPlayHookIDs::SetCursorPosHookID },
		{ "Get Key State", true, "Get Key State", SplitPlayHookIDs::GetKeyStateHookID },
		{ "Get Async Key State", true, "Get Async Key State", SplitPlayHookIDs::GetAsyncKeyStateHookID },
		{ "Get Keyboard State", true, "Get Keyboard State", SplitPlayHookIDs::GetKeyboardStateHookID },
		{ "Cursor Visibility", true, "Cursor Visibility", SplitPlayHookIDs::CursorVisibilityStateHookID },
		{ "Clip Cursor", true, "Clip Cursor", SplitPlayHookIDs::ClipCursorHookID },
		{ "Focus", true, "Focus", SplitPlayHookIDs::FocusHooksHookID },
		{ "Rename Handles", true, "Rename Handles", SplitPlayHookIDs::RenameHandlesHookID },
		{ "Block Raw Input", false, "Block Raw Input", SplitPlayHookIDs::BlockRawInputHookID },
		{ "Dinput Order", false, "Dinput Order", SplitPlayHookIDs::DinputOrderHookID },
		{ "Xinput", false, "Xinput", SplitPlayHookIDs::XinputHookID }		
	};

	std::vector<ProfileOption> messageFilters
	{
		{ "Raw Input", true, "Raw Input", SplitPlayMessageFilterIDs::RawInputFilterID },
		{ "Mouse Move", true, "Mouse Move", SplitPlayMessageFilterIDs::MouseMoveFilterID },
		{ "Mouse Activate", true, "Mouse Activate", SplitPlayMessageFilterIDs::MouseActivateFilterID },
		{ "Window Activate", true, "Window Activate", SplitPlayMessageFilterIDs::WindowActivateFilterID },
		{ "Window Activate App", true, "Window Activate App", SplitPlayMessageFilterIDs::WindowActivateAppFilterID },
		{ "Mouse Wheel Filter", true, "Mouse Wheel Filter", SplitPlayMessageFilterIDs::MouseWheelFilterID },
		{ "Mouse Button", true, "Mouse Button", SplitPlayMessageFilterIDs::MouseButtonFilterID },
		{ "Keyboard Button", true, "Keyboard Button", SplitPlayMessageFilterIDs::KeyboardButtonFilterID },
	};

	bool dinputToXinputRedirection = false;
	bool useOpenXinput = false;

	bool useFakeClipCursor = true;

	bool showCursorWhenImageUpdated = false;
	
	bool drawFakeMouseCursor = true;
	bool allowMouseOutOfBounds = false;
	bool extendMouseBounds = false;
	bool toggleFakeCursorVisibilityShortcut = false;
	bool sendMouseMovementMessages = true;
	bool sendMouseButtonMessages = true;
	bool sendMouseWheelMessages = true;
	bool sendKeyboardButtonMessages = true;
	
	bool focusMessageLoop = true;
	bool focusLoopSendWM_ACTIVATE = true;
	bool focusLoopSendWM_NCACTIVATE = false;
	bool focusLoopSendWM_ACTIVATEAPP = false;
	bool focusLoopSendWM_SETFOCUS = true;
	bool focusLoopSendWM_MOUSEACTIVATE = true;

	std::vector<std::string> renameHandles{};
	std::vector<std::string> renameNamedPipeHandles{};

	std::vector<unsigned int> blockedMessages;
	
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp("hooks", hooks),
			cereal::make_nvp("messageFilters", messageFilters),

			cereal::make_nvp("renameHandles", renameHandles),
			cereal::make_nvp("renameNamedPipeHandles", renameNamedPipeHandles),

			cereal::make_nvp("dinputToXinputRedirection", dinputToXinputRedirection),
			cereal::make_nvp("useOpenXinput", useOpenXinput),
			cereal::make_nvp("useFakeClipCursor", useFakeClipCursor),
			cereal::make_nvp("showCursorWhenImageUpdated", showCursorWhenImageUpdated),

			cereal::make_nvp("drawFakeMouseCursor", drawFakeMouseCursor),
			cereal::make_nvp("allowMouseOutOfBounds", allowMouseOutOfBounds),
			cereal::make_nvp("extendMouseBounds", extendMouseBounds),
			cereal::make_nvp("toggleFakeCursorVisibilityShortcut", toggleFakeCursorVisibilityShortcut),
			cereal::make_nvp("sendMouseMovementMessages", sendMouseMovementMessages),
			cereal::make_nvp("sendMouseButtonMessages", sendMouseButtonMessages),
			cereal::make_nvp("sendMouseWheelMessages", sendMouseWheelMessages),
			cereal::make_nvp("sendKeyboardButtonMessages", sendKeyboardButtonMessages),

			cereal::make_nvp("focusMessageLoop", focusMessageLoop),
			cereal::make_nvp("focusLoopSendWM_ACTIVATE", focusLoopSendWM_ACTIVATE),
			cereal::make_nvp("focusLoopSendWM_NCACTIVATE", focusLoopSendWM_NCACTIVATE),
			cereal::make_nvp("focusLoopSendWM_ACTIVATEAPP", focusLoopSendWM_ACTIVATEAPP),
			cereal::make_nvp("focusLoopSendWM_SETFOCUS", focusLoopSendWM_SETFOCUS),
			cereal::make_nvp("focusLoopSendWM_MOUSEACTIVATE", focusLoopSendWM_MOUSEACTIVATE),


			cereal::make_nvp("blockedMessages", blockedMessages)
		);
	}

	static void SaveToFile(const Profile& data, const std::string& name);
	static void LoadFromFile(Profile& data, const std::string& name);
	static bool DoesProfileFileAlreadyExist(const std::string& name);
	static std::vector<std::string> GetAllProfiles();

	// A ready-to-use profile for playing a game on a second screen (e.g. SpaceDesk display)
	// using only a controller, while the main screen keeps the real keyboard and mouse.
	static Profile MakeSecondScreenControllerProfile();
};

}