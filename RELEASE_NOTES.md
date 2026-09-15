# SplitPlay 1.3.1

Cameras and microphones are now shown, and you can identify every device at a glance.

## What's new

- **Cameras are listed.** Every connected camera appears in the device list. A camera's row lights up green
  while an app is using it, so you can tell which one is live. (Identify-only for now.)

- **Microphones are listed.** Every connected microphone appears, default first. A microphone's row lights
  up green while sound is being picked up, using a live peak meter.

- **Click an audio output to hear it.** Clicking an audio output row plays a short test tone **on that
  specific device**, so you can confirm which speaker/headphone/cable it is.

- **Click a display to see it.** Clicking a display row flashes a big "Display 1 / 2 / 3" label on that
  physical monitor for a couple of seconds.

- **Displays are numbered** in the list ("Display 1", "Display 2", ...) to match the on-screen flash.

## Everything from before

- Add several apps at once — each with its own display, devices and audio, running side by side.
- A device (mouse / keyboard / controller) belongs to only one app at a time.
- One display assigned = window locked to it. Several = it may move between them, but nowhere else.
- Per-app audio routing to any number of output devices (route-only, Windows 10 2004+).
- Assigned devices are released automatically when an app is closed, killed, or crashes.
- Remove an app with the **Remove** button or right-click in the sidebar.

## Quick start

1. Plug in your controller (Xbox / XInput, or a DirectInput pad).
2. Make sure your second screen is **extended** (not mirrored).
3. Unzip and run `SplitPlay.exe`.
4. Press **+ Add app**, choose the app/game, the display(s), the audio output(s), assign devices, then
   **Start this app**.
5. Add more apps if you like, then open the **Running** tab to manage them.

## Files

The zip contains a single `SplitPlay` folder:

```
SplitPlay/
  SplitPlay.exe
  SplitPlayHooks32.dll / SplitPlayHooks64.dll
  SplitPlayLoader32.dll / SplitPlayLoader64.dll
  SplitPlayIJ32.exe / SplitPlayIJ64.exe
  SplitPlayIJP32.dll / SplitPlayIJP64.dll
  README.md
```

Keep all files together. Runs on 64-bit Windows 10/11; 32-bit target apps supported too.
