<div align="center">

# 🎮 SplitPlay

**Send any app to another screen and control it with the devices you choose — while your main keyboard and mouse keep working normally.**

SplitPlay lets one PC behave like several: multiple apps, each on its own screen, each with its own controller, mouse, keyboard and speakers.

[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-0078D6?logo=windows&logoColor=white)](#)
[![Architecture](https://img.shields.io/badge/arch-x64%20%7C%20x86-informational)](#)
[![Portable](https://img.shields.io/badge/portable-no%20install-22c55e)](#)
[![License](https://img.shields.io/badge/license-MIT-blue)](#license)

</div>

---

## ✨ What is SplitPlay?

Have you ever wanted to play a game on a spare screen or tablet while your main screen stays free for
work, chat or browsing? Or run two games at once, each with its own controller?

**SplitPlay does exactly that.**

It gently hooks into the apps you choose and gives them a *private set of devices*: the controller you
assigned, the mouse you assigned, the keyboard you assigned, and the screen and speakers you assigned.
Everything else on your PC keeps working exactly as before.

> One computer. Several "players". No extra hardware, no install, no reboot.

<div align="center">

| 🖥️ Move the window | 🎮 Pick the controller | 🖱️⌨️ Share only what you want | 🔊 Route the sound |
|:---:|:---:|:---:|:---:|
| Lock an app to one screen, or let it roam between several | Any Xbox / XInput / DirectInput pad | Assigned devices go to the app, the rest stay on your desktop | Send an app's audio to the output you choose |

</div>

---

## 🚀 Why people use it

| Use case | How SplitPlay helps |
|---|---|
| 🎯 **Second-screen gaming** | Play on a tablet (e.g. SpaceDesk) or TV while the main monitor stays usable |
| 🕹️ **Two games at once** | Run two apps side by side, each with its own controller and screen |
| 🎬 **Media or emulator on the side** | Keep a controller-driven app on the TV and still type on your PC |
| 🧑‍💻 **AFK / idle apps** | Keep a game or tool receiving controller input even when it isn't focused |
| 🔊 **Private audio** | Send one app's sound to headphones while everything else uses speakers |
| 🎥 **See what's plugged in** | Every mouse, keyboard, controller, camera, microphone, display and audio output, in one list |

### Identify any device at a glance

Not sure which "USB Input Device" is which? Just use it:

| Device | What happens |
|---|---|
| 🖱️ Mouse | Row lights up when you move it |
| ⌨️ Keyboard | Row lights up when you press a key |
| 🎮 Controller | Row lights up when you move a stick or press a button |
| 🎙️ Microphone | Row lights up when sound is detected |
| 🎥 Camera | Row lights up while an app is using it |
| 🔊 Audio output | **Click it** to play a test tone **on that device** |
| 🖥️ Display | **Click it** to flash "Display 1/2/3" on that screen |

---

## 📦 Quick start

1. **Download** the latest `SplitPlay.zip` from [Releases](../../releases) and unzip it anywhere.
2. **Plug in** your controller (Xbox / XInput, or a DirectInput pad).
3. Make sure your second screen is **extended**, not mirrored.
   *(Windows: Settings → System → Display → Extend these displays)*
4. **Run** `SplitPlay.exe`.
5. Press **➕ Add app** in the left sidebar and set it up in the **Setup** tab:

   | Step | What to do |
   |---|---|
   | **1. Choose the app or game** | Browse to an `.exe`, or attach to an app that's already running |
   | **2. Choose the display(s)** | One screen = locked there. Several = the window may move between them. Click a row to flash its number |
   | **2b. Choose audio output(s)** | Send the app's sound to any output device(s). Click a row to hear a test tone on it |
   | **3. Assign devices** | Click a mouse / keyboard / controller to switch it ON for this app. Cameras and microphones are listed for identification |
   | **4. Start** | Press **Start this app** |

6. Want another app at the same time? Press **➕ Add app** again. Each app is independent.
7. Open the **Running** tab to see everything at a glance and start/stop each app.

---

## 🧠 How it actually works

SplitPlay injects a small set of hooks into each target process. Those hooks:

- keep the app running and receiving input even when it **isn't** the focused window,
- forward **only the devices you switched ON** to that app,
- hide the real keyboard/mouse from the app so they keep controlling Windows,
- move the window onto the display(s) you chose and keep it there,
- route the app's audio to the output device(s) you chose.

<div align="center">

```text
        YOUR PC                                    APP  (e.g. a game)
 ┌────────────────────┐                         ┌────────────────────┐
 │  Main screen       │                         │  Assigned screen   │
 │  (desktop, work)   │                         │  (game window)     │
 └────────────────────┘                         └────────────────────┘
   Mouse A  Keyboard A      ──── assigned ───►     receives these
   Mouse B  Keyboard B      ──── NOT assigned ──►  stays on desktop
   Controller 1             ──── assigned ───►     controls the game
   Speakers (main)          ──── NOT assigned ──►  keeps system audio
   Headphones               ──── assigned ───►     plays the game audio
```

</div>

---

## 🔒 What "assigned" really means

### Input devices belong to one app

A mouse, keyboard or controller can be assigned to **only one app at a time**. Switch it ON for one app
and it's automatically switched OFF everywhere else — so a click or key press can never land in the wrong
window. Devices you leave OFF keep controlling Windows as usual.

### Find out which device is which

Move a mouse, press a key, or wiggle your controller — the matching row **lights up green** 🟢 so you know
exactly which physical device each entry is, before you toggle it.

- **Microphones** light up while sound is coming in.
- **Cameras** light up while an app is using them.
- **Click an audio output** to play a short test tone **on that device**, so you can hear which one it is.
- **Click a display** to flash its number ("Display 1", "Display 2") on that physical screen.

### Cameras and microphones are listed too

SplitPlay shows every connected **camera** and **microphone** in the device list so you can see what is
plugged in and identify it. These are **identify-only** for now — they are not yet redirected into the app.

### Displays: one means *locked*, several mean *roam*

- **One display selected** → the window is locked to it and cannot be moved off it.
- **Several displays selected** → the window may be moved between those screens, but never onto a screen
  you didn't assign.

### Audio: route-only

When you assign an output device, the app's sound is captured and replayed to **that device**. This is
*route-only*: Windows has no way to stop other apps from using a physical speaker without a custom audio
driver, so SplitPlay doesn't pretend to. It sends **this app's** audio where you asked — nothing else
changes. Requires Windows 10 version 2004 or newer.

### It cleans up after itself

The moment the app is closed, killed from the taskbar, or crashes, SplitPlay notices, **releases the
devices**, and stops routing audio. Your mouse and keyboard are never left stuck on the app. You can also
press **Stop** any time. The setup stays until you remove it.

### Deleting an app

Select it and press **Remove** at the top of the Setup tab, or **right-click** its row in the sidebar and
choose *Remove app*. Stop the app first.

---

## 🎛️ Controls

| Key | Action |
|---|---|
| `Right Ctrl + Right Alt + 1` | Open the in-app hook overlay for advanced tweaking |
| `End` | Lock / unlock input (handy while setting up) |
| **Home** | Toggle the fake in-game cursor (if enabled) |

Keep `SplitPlay.exe` running while your apps run — it owns the connection to each target process.

---

## ⚠️ Good to know

- Run fullscreen games in **windowed** or **borderless** mode. Exclusive fullscreen can ignore being moved
  to another display.
- DirectInput controllers use a Dinput-to-Xinput translation; both triggers cannot be analog at once.
- Per-app audio uses **process loopback**, available on **Windows 10 2004+**. On older builds, audio is
  simply left untouched.
- Some apps refuse to be repositioned — windowed/borderless usually fixes it.

---

## 🛠️ Building from source

**Requirements:** Visual Studio with the **Desktop development with C++** workload (C++17) and the
Windows 10/11 SDK.

The solution is at `src/SplitPlay/SplitPlay.sln`.

To produce the portable release zip (builds x64 + x86 and packages everything):

```powershell
powershell -ExecutionPolicy Bypass -File tools\build-release.ps1
```

This creates `SplitPlay.zip` containing a single `SplitPlay` folder with `SplitPlay.exe` and the hook DLLs.

---

## 📄 License

MIT. See `LICENSE` — includes the original ProtoInput copyright notice, as required.
