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

// A Windows audio input (a "capture endpoint": microphones, line-in, etc.)
struct AudioInputInfo
{
	std::wstring id;
	std::wstring name;          // friendly name, e.g. "Microphone (Realtek Audio)"
	std::wstring description;
	bool isDefault = false;
};

// Enumerates all active audio output devices, default first.
std::vector<AudioOutputInfo> EnumerateAudioOutputs();

// Enumerates all active audio input (microphone) devices, default first.
std::vector<AudioInputInfo> EnumerateAudioInputs();

// ---- Identify helpers (used by the UI to show "which one is this?") ----------

// Plays a short test tone on the given output device (used when you click an audio row).
// Returns true when the tone was actually rendered to that device.
// Blocking (roughly the tone duration); call from a worker thread.
bool PlayTestTone(const std::wstring& outputDeviceId);

// ---- Live microphone activity ------------------------------------------------

// Starts a lightweight peak meter on the given microphone. Safe to call repeatedly.
void WatchMicrophone(const std::wstring& inputDeviceId);

// Returns the most recent peak level (0.0 - 1.0) for a watched microphone.
float GetMicrophoneLevel(const std::wstring& inputDeviceId);

// Stops watching microphones that are no longer in the list.
void PruneMicrophoneWatches(const std::vector<std::wstring>& keepIds);

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
