# SplitPlay 1.0.0

Send an app or game to a **second screen** and control it with the devices you choose, while your
keyboard and mouse keep working on the main screen.

## Features

- **Modern dashboard UI** - clean dark theme, real system font, cards and toggles instead of the old 1990s look.
- **Works with any app, not just games** - attach to any running window, or launch any `.exe`.
- **Device assignment** - turn each controller / mouse / keyboard ON or OFF for the target app.
  Devices show their real product name and an icon.
- **Find which device is which** - move a mouse or press a key and that row **lights up green**,
  so you can tell exactly which physical device an entry refers to before enabling it.
- **Second-screen placement** - pick the display and SplitPlay moves/resizes the window onto it.
- **Keeps the app alive while unfocused** - the app keeps running and receiving input while you use Windows.
- **Portable** - no installer. Unzip and run `SplitPlay.exe`.

## Quick start

1. Plug in your controller (Xbox / XInput, or a DirectInput pad such as a Fantech Shooter).
2. Make sure your second screen is connected and **extended** (not mirrored).
3. Unzip `SplitPlay.zip` and run `SplitPlay.exe`.
4. Choose the app or game, choose the display, assign devices, press **Start**.

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

Keep all files together in the folder. Runs on 64-bit Windows 10/11; 32-bit target apps are supported too.

## Notes

- Run fullscreen games in **windowed** or **borderless** mode - exclusive fullscreen can ignore being moved.
- DirectInput controllers use a Dinput-to-Xinput translation (both triggers cannot be analog at once).
- Built on the open-source ProtoInput hooking library (MIT).
