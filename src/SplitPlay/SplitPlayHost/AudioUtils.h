#pragma once
#include <string>
#include <vector>

namespace SplitPlayHost
{

// A Windows audio output (a "render endpoint": speakers, headphones, a virtual cable, etc.)
struct AudioOutputInfo
{
	std::wstring id;            // IMMDevice device id (stable)
	std::wstring name;          // friendly name, e.g. "Speakers (Realtek Audio)"
	std::wstring description;   // device description / adapter name
	bool isDefault = false;     // is this the system default output?
};

// Enumerates all active audio output devices, default first.
std::vector<AudioOutputInfo> EnumerateAudioOutputs();

// ---- Per-app audio routing ----------------------------------------------------
// Uses WASAPI process loopback (Windows 10 2004+): we capture only the target
// process's audio and render it to the chosen output device(s). This is "route-only":
// other apps are not blocked from the physical device, but this app's sound is sent
// to exactly the device(s) you chose.
//
// Returns true if routing started (or is unavailable because the OS is too old).
bool StartAudioRouting(unsigned long pid, const std::vector<std::wstring>& outputDeviceIds);

// Stops any running audio routing for the given pid.
void StopAudioRouting(unsigned long pid);

// Stops all audio routing (used when the host shuts down).
void StopAllAudioRouting();

}
