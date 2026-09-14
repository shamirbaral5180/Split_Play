#pragma once
#include <vector>
#include "Hook.h"
#include "InstallHooks.h"
#include "splitplayloader.h"

namespace SplitPlay
{

//NOTE: To add hook IDs, change splitplayloader.h in SplitPlayLoader\include

class HookManager
{
private:
	std::vector<std::unique_ptr<Hook>> hooks{};

	static HookManager hookManagerInstance;
	
	template<typename T>
	void AddHook(SplitPlayHookIDs index)
	{
		if (hooks.size() != index)
			std::cerr << "Trying to call AddHook with an invalid ID" << std::endl;
		else
		{
			hooks.push_back(std::make_unique<T>());
		}
	}
	
	HookManager();
	
public:
	static const std::vector<std::unique_ptr<Hook>>& GetHooks()
	{
		return hookManagerInstance.hooks;
	}

	static void InstallHook(SplitPlayHookIDs hookID);
	static void UninstallHook(SplitPlayHookIDs hookID);
	static bool IsInstalled(SplitPlayHookIDs hookID);
};

}
