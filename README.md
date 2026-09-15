# SplitPlay

Send an app or game to a **second screen** (for example a SpaceDesk display) and control it with the
devices you choose, while your keyboard and mouse stay on the main screen and keep working normally.

SplitPlay is a portable, no-install tool. Unzip it, run `SplitPlay.exe`, pick your app, your display and
your devices, and press **Start**.

## How it works

SplitPlay injects a small set of hooks into the target process. Those hooks:

- Keep the app running and receiving input even when it is **not** the focused window.
- Send only the **devices you switch ON** to the app.
- Hide the real keyboard/mouse from the app so they keep controlling Windows.
- Optionally move and resize the window onto the display you choose.

This is built on top of the open-source SplitPlay hooking library (originally ProtoInput).

## Quick start

1. Plug in your controller (Xbox / XInput, or a DirectInput pad such as a Fantech Shooter).
2. Make sure your second screen is connected and extended (not mirrored).
3. Run `SplitPlay.exe`.
4. In the app:
   1. **Choose the app or game** - browse to an `.exe`, or pick it from the list of running apps
      (search by name or window title).
   2. **Choose the display** - select the screen the window should open on.
   3. **Assign devices** - click a device to switch it ON/OFF for the app:
      - **Controller** - pick the pad the app should use.
      - **Mouse** / **Keyboard** - optional. Leave them OFF to keep them on the desktop.
      Devices show their real product name, an icon, and an ON/OFF badge.
   4. Press **Start**.
5. While the app runs, use the rest of Windows normally - only the devices you switched ON control it.

### Find out which device is which

Move a mouse, press a key, or move your controller, and the matching row **lights up green** so you know
exactly which physical device that entry refers to before you turn it on or off.

### Devices belong to the app

When you switch a mouse or keyboard ON for an app, SplitPlay **binds that device to the app**: its clicks
and key presses go only to the app and no longer leak to whatever else is on screen. Everything you left
OFF keeps controlling Windows normally.

### The window stays on its display

Once you assign a display, SplitPlay moves the app window there and **keeps it locked** to that display
and size. If the app tries to move or resize itself, it is put back.

## Controls

- `Right Ctrl + Right Alt + 1` opens the in-app overlay so you can tweak hooks and filters live.
- `Home` locks/unlocks input (useful while setting up).
- Keep `SplitPlay.exe` running while the app runs - it owns the connection to the target process.

## Building from source

Requirements: Visual Studio with the **Desktop development with C++** workload (C++17), Windows 10/11 SDK.

The solution is at `src/SplitPlay/SplitPlay.sln`.

To produce a portable release zip (builds x64 + x86 and packages everything):

```powershell
powershell -ExecutionPolicy Bypass -File tools\build-release.ps1
```

This creates `SplitPlay.zip` containing a single `SplitPlay` folder with `SplitPlay.exe` and the hook DLLs.

## Notes

- Run fullscreen games in **windowed** or **borderless** mode. Exclusive fullscreen can ignore being moved
  to another display.
- DirectInput controllers use a Dinput-to-Xinput translation; both triggers cannot be analog at once.
- A few apps refuse to be repositioned - use borderless/windowed mode.

## License

MIT. See `LICENSE` (includes the original ProtoInput copyright notice, as required).
