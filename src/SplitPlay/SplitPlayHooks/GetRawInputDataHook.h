#pragma once
#include "Hook.h"
#include "InstallHooks.h"

namespace SplitPlay
{

class GetRawInputDataHook final : public Hook
{
private:
	HookInfo hookInfo{};

public:
	const char* GetHookName() const override { return "Get Raw Input Data"; }
	const char* GetHookDescription() const override
	{
		return
		"This hook forwards the raw input received by SplitPlay Input to the game. "
		"This is required for any game that uses raw input, or SplitPlay Input will consume all the input.";
	}
	
	bool HasGuiStatus() const override { return false; }
	// void ShowGuiStatus() override;
	void InstallImpl() override;
	void UninstallImpl() override;
};

}
