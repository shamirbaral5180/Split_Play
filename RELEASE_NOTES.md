# SplitPlay 1.3.0

Output devices, automatic cleanup, and a friendlier look.

## What's new

- **Assign audio output devices.** Each app now has an "Audio output(s)" section listing every connected
  output device (speakers, headphones, virtual cables). Tick one or more and that app's sound is routed to
  them. Leave them all OFF to keep normal system audio.
  - *Route-only:* the app's audio is sent to the chosen device(s); other apps are not blocked from the
    physical device (Windows cannot do that without a custom audio driver, and SplitPlay does not pretend
    to). Uses WASAPI process loopback, so **Windows 10 2004 or newer** is required. On older builds audio
    is simply left untouched.

- **Assign several displays to one app.**
  - **One display selected** → the window is locked to it and cannot be moved off it.
  - **Several displays selected** → the window may be moved between those screens, but never onto a screen
    you did not assign.

- **Automatic cleanup when an app exits.** SplitPlay watches each target process. The moment it is closed,
  killed from the taskbar, or crashes, SplitPlay releases the assigned devices and stops audio routing.
  Your mouse and keyboard are never left swallowed system-wide. Setups remain until you remove them.

- **Nicer README** with a clear explanation of what SplitPlay is, who it is for, and how to use it.

## Everything from before

- Add several apps at once — each with its own display, devices and audio, running side by side.
- A device (mouse / keyboard / controller) belongs to only one app at a time.
- Rows light up green when you use a device so you can tell which is which.
- App windows stay on their assigned display.
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
