# SplitPlay 1.1.0

Multi-app support. You can now add several apps at once, each with its own display and its own devices,
running side by side.

## What's new

- **Add multiple apps.** The left sidebar lists every app you have added, each with a status dot (green
  while running). Use **+ Add app** to create more.
- **Tabs.** The main area now has a **Setup** tab (configure the selected app in 4 steps) and a
  **Running** tab (all apps with their display and devices, and per-app Start/Stop).
- **Each app is independent.** Every app gets its own target display and its own devices, and is locked to
  its own screen. Start and stop them individually.
- **A device belongs to one app.** A mouse, keyboard or controller can be assigned to only one app at a
  time - switching it ON for one app automatically switches it OFF for all others, so input never leaks
  into the wrong window.
- **Delete an app.** Press **Remove** at the top of the Setup tab, or **right-click** the app's row in the
  sidebar and choose *Remove app*. Apps must be stopped before they can be removed.

## Everything from before

- Keyboard, mouse and controller rows light up green when you use them, so you know which device is which.
- Assigned devices are bound to the app and no longer leak input to other windows.
- App windows are moved to, and kept locked on, their assigned display.

## Quick start

1. Plug in your controller (Xbox / XInput, or a DirectInput pad).
2. Make sure your second screen is **extended** (not mirrored).
3. Unzip and run `SplitPlay.exe`.
4. Press **+ Add app**, choose the app/game, the display, and assign devices, then **Start this app**.
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
