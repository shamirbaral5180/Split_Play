#include "Gui.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <imgui_internal.h>
#include <string>
#include <vector>
#include "Instance.h"
#include <filesystem>
#include <BlackBone/Process/Process.h>
#include <iostream>
#include "RawInput.h"
#include "splitplayloader.h"
#include "splitplayutil.h"
#include "Profiles.h"
#include "RawInput.h"
#include "DisplayUtils.h"
#include "ControllerUtils.h"
#include "DeviceUtils.h"
#include "UiTheme.h"
#include "SimpleMode.h"

namespace SplitPlayHost
{

std::wstring dllFolderPath{};

std::vector<SplitPlayInstanceHandle> trackedInstanceHandles{};

static bool isInputCurrentlyLocked = false;
void OnInputLockChange(bool locked)
{
    isInputCurrentlyLocked = locked;

    bool freeze = !isInputCurrentlyLocked && freezeGameInputWhileInputNotLocked;

    for (const auto& instanceHandle : trackedInstanceHandles)
        SetExternalFreezeFakeInput(instanceHandle, freeze);
}

static void PushDisabledLocal()
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
}

static void PopDisabledLocal()
{
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}

void ConfigureInstance(SplitPlayInstanceHandle instanceHandle, Instance& instance, const Profile& profile, int index,
                       bool setWindowPosition, int windowX, int windowY, int windowWidth, int windowHeight)
{
    SetupState(instanceHandle, index);

    // Bit lazy but it works 
    auto hookEnabled = [&profile](unsigned int id)
    {
        for (const auto& hook : profile.hooks)
        {
            if (hook.enabled && hook.id == id)
                return true;
        }

        return false;
    };

    auto filterEnabled = [&profile](unsigned int id)
    {
        for (const auto& filter : profile.messageFilters)
        {
            if (filter.enabled && filter.id == id)
                return true;
        }

        return false;
    };

    if (hookEnabled(RegisterRawInputHookID))        InstallHook(instanceHandle, RegisterRawInputHookID);
    if (hookEnabled(GetRawInputDataHookID))         InstallHook(instanceHandle, GetRawInputDataHookID);
    if (hookEnabled(MessageFilterHookID))           InstallHook(instanceHandle, MessageFilterHookID);
    if (hookEnabled(GetCursorPosHookID))            InstallHook(instanceHandle, GetCursorPosHookID);
    if (hookEnabled(SetCursorPosHookID))            InstallHook(instanceHandle, SetCursorPosHookID);
    if (hookEnabled(GetKeyStateHookID))             InstallHook(instanceHandle, GetKeyStateHookID);
    if (hookEnabled(GetAsyncKeyStateHookID))        InstallHook(instanceHandle, GetAsyncKeyStateHookID);
    if (hookEnabled(GetKeyboardStateHookID))        InstallHook(instanceHandle, GetKeyboardStateHookID);

    SetShowCursorWhenImageUpdated(instanceHandle, profile.showCursorWhenImageUpdated);
    if (hookEnabled(CursorVisibilityStateHookID))   InstallHook(instanceHandle, CursorVisibilityStateHookID);

    SetCursorClipOptions(instanceHandle, profile.useFakeClipCursor);
    if (hookEnabled(ClipCursorHookID))              InstallHook(instanceHandle, ClipCursorHookID);

    if (hookEnabled(FocusHooksHookID))              InstallHook(instanceHandle, FocusHooksHookID);
    if (hookEnabled(RenameHandlesHookID))           InstallHook(instanceHandle, RenameHandlesHookID);

    if (hookEnabled(BlockRawInputHookID))           InstallHook(instanceHandle, BlockRawInputHookID);

    SetUseOpenXinput(instanceHandle, profile.useOpenXinput);
    SetUseDinputRedirection(instanceHandle, profile.dinputToXinputRedirection);
    if (hookEnabled(XinputHookID))                  InstallHook(instanceHandle, XinputHookID);

    if (hookEnabled(DinputOrderHookID))             InstallHook(instanceHandle, DinputOrderHookID);

    if (hookEnabled(SetWindowPosHookID))
    {
        InstallHook(instanceHandle, SetWindowPosHookID);

        if (setWindowPosition)
            SetSetWindowPosSettings(instanceHandle, windowX, windowY, windowWidth, windowHeight);
    }

    if (filterEnabled(RawInputFilterID))            EnableMessageFilter(instanceHandle, RawInputFilterID);
    if (filterEnabled(MouseMoveFilterID))           EnableMessageFilter(instanceHandle, MouseMoveFilterID);
    if (filterEnabled(MouseActivateFilterID))       EnableMessageFilter(instanceHandle, MouseActivateFilterID);
    if (filterEnabled(WindowActivateFilterID))      EnableMessageFilter(instanceHandle, WindowActivateFilterID);
    if (filterEnabled(WindowActivateAppFilterID))   EnableMessageFilter(instanceHandle, WindowActivateAppFilterID);
    if (filterEnabled(MouseWheelFilterID))          EnableMessageFilter(instanceHandle, MouseWheelFilterID);
    if (filterEnabled(MouseButtonFilterID))         EnableMessageFilter(instanceHandle, MouseButtonFilterID);
    if (filterEnabled(KeyboardButtonFilterID))      EnableMessageFilter(instanceHandle, KeyboardButtonFilterID);


    for (const auto msg : profile.blockedMessages)
    {
        EnableMessageBlock(instanceHandle, msg);
    }

    SetupMessagesToSend(instanceHandle,
                        profile.sendMouseWheelMessages,
                        profile.sendMouseButtonMessages,
                        profile.sendMouseMovementMessages,
                        profile.sendKeyboardButtonMessages);

    if (profile.focusMessageLoop)
        StartFocusMessageLoop(instanceHandle,
                              5,
                              profile.focusLoopSendWM_ACTIVATE,
                              profile.focusLoopSendWM_ACTIVATEAPP,
                              profile.focusLoopSendWM_NCACTIVATE,
                              profile.focusLoopSendWM_SETFOCUS,
                              profile.focusLoopSendWM_MOUSEACTIVATE);

    SetDrawFakeCursor(instanceHandle, profile.drawFakeMouseCursor);

    AllowFakeCursorOutOfBounds(instanceHandle, profile.allowMouseOutOfBounds, profile.extendMouseBounds);

    SetToggleFakeCursorVisibilityShortcut(instanceHandle, profile.toggleFakeCursorVisibilityShortcut, VK_HOME);

    for (const auto& renameHandle : profile.renameHandles)
        AddHandleToRename(instanceHandle, utf8_decode(renameHandle).c_str());

    for (const auto& renameNamedPipeHandle : profile.renameNamedPipeHandles)
        AddNamedPipeToRename(instanceHandle, utf8_decode(renameNamedPipeHandle).c_str());

    if (instance.mouseHandle != -1)
        AddSelectedMouseHandle(instanceHandle, instance.mouseHandle);

    if (instance.keyboardHandle != -1)
        AddSelectedKeyboardHandle(instanceHandle, instance.keyboardHandle);

    SetControllerIndex(instanceHandle, instance.controllerIndex);

    SetExternalFreezeFakeInput(instanceHandle, !isInputCurrentlyLocked && freezeGameInputWhileInputNotLocked);

    if (!instance.runtime)
        WakeUpProcess(instanceHandle);
}


void FirstTimeSetup()
{
    InitialiseRawInput();

    wchar_t pathchars[MAX_PATH];
    GetModuleFileNameW(NULL, pathchars, MAX_PATH);
    dllFolderPath = pathchars;
    size_t pos = dllFolderPath.find_last_of(L"\\");
    if (pos != std::string::npos)
        dllFolderPath = dllFolderPath.substr(0, pos + 1);
}

void RenderImgui()
{
    // ImGui::ShowDemoWindow();
    // return;

    if (static bool firstTimeSetup = true; firstTimeSetup)
    {
        firstTimeSetup = false;
        FirstTimeSetup();
        RefreshSimpleModeDevices();

        // Start with one empty app ready to configure
        if (GetAppState().instances.empty())
            AddInstance();
    }

    const auto displaySize = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, displaySize.y), ImGuiCond_Always);

    if (ImGui::Begin("Main", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar |
                     //ImGuiWindowFlags_NoBackground |
                     // ImGuiWindowFlags_MenuBar |
                     ImGuiWindowFlags_NoBringToFrontOnFocus
    ))
    {
        RenderSimpleMode();
    }
    ImGui::End();
}

// Builds the Instance for a config and injects, wiring up hooks and devices.
bool StartInstance(int id)
{
    auto& state = GetAppState();

    InstanceConfig* cfg = nullptr;
    for (auto& instance : state.instances)
    {
        if (instance.id == id)
        {
            cfg = &instance;
            break;
        }
    }

    if (cfg == nullptr)
        return false;

    if (cfg->hasInjected)
    {
        cfg->statusMessage = "Already running. Stop it first.";
        return false;
    }

    const bool canStart = cfg->launchNewInstance ? !cfg->gameFilepath.empty() : cfg->runningPid != 0;
    if (!canStart)
    {
        cfg->statusMessage = "Choose an app or game first.";
        return false;
    }

    // Build the instance
    Instance instance = cfg->launchNewInstance
        ? Instance(cfg->gameFilepath, std::filesystem::path(cfg->gameFilepath).filename().wstring())
        : Instance(cfg->runningPid, cfg->runningProcessName);

    // Profile: second-screen controller preset, then apply the user's device assignments
    auto profile = Profile::MakeSecondScreenControllerProfile();

    if (cfg->selectedControllerIndex >= 0 && cfg->selectedControllerIndex < (int)state.controllers.size() &&
        cfg->selectedControllerIndex < (int)cfg->controllerEnabled.size() &&
        cfg->controllerEnabled[cfg->selectedControllerIndex])
    {
        const auto& controller = state.controllers[cfg->selectedControllerIndex];
        instance.controllerIndex = controller.index;
        profile.dinputToXinputRedirection = controller.requiresDinputRedirection;
        profile.useOpenXinput = controller.requiresOpenXinput;
    }
    else
    {
        instance.controllerIndex = 0;
    }

    // Mouse / keyboard assigned to the game (optional)
    if (cfg->selectedMouseIndex >= 0 && cfg->selectedMouseIndex < (int)state.mice.size() &&
        cfg->selectedMouseIndex < (int)cfg->mouseEnabled.size() &&
        cfg->mouseEnabled[cfg->selectedMouseIndex])
    {
        instance.mouseHandle = state.mice[cfg->selectedMouseIndex].handle;
    }
    else
    {
        instance.mouseHandle = -1;
    }

    if (cfg->selectedKeyboardIndex >= 0 && cfg->selectedKeyboardIndex < (int)state.keyboards.size() &&
        cfg->selectedKeyboardIndex < (int)cfg->keyboardEnabled.size() &&
        cfg->keyboardEnabled[cfg->selectedKeyboardIndex])
    {
        instance.keyboardHandle = state.keyboards[cfg->selectedKeyboardIndex].handle;
    }
    else
    {
        instance.keyboardHandle = -1;
    }

    profile.drawFakeMouseCursor = cfg->showFakeCursor;

    // Inject
    SplitPlayInstanceHandle instanceHandle = 0;
    unsigned long pid = -1;

    if (instance.runtime)
    {
        instanceHandle = RemoteLoadLibraryInjectRuntime(instance.pid, dllFolderPath.c_str());
    }
    else
    {
        instanceHandle = EasyHookInjectStartup(instance.filepath.c_str(), L"", 0, dllFolderPath.c_str(), &pid);
    }

    if (instanceHandle == 0)
    {
        cfg->statusMessage = "Injection failed. See console for details.";
        return false;
    }

    trackedInstanceHandles.push_back(instanceHandle);
    cfg->instanceHandle = instanceHandle;

    // Window placement across the assigned monitors.
    // One monitor  -> lock the window to it.
    // Two or more -> use their combined bounds and let the window move inside them.
    bool setWindowPos = false;
    bool lockWindow = false;
    int wx = 0, wy = 0, ww = 0, wh = 0;

    if (cfg->moveWindowToDisplay && !state.monitors.empty())
    {
        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
        int assignedCount = 0;

        for (int i = 0; i < (int)state.monitors.size() && i < (int)cfg->monitorEnabled.size(); ++i)
        {
            if (!cfg->monitorEnabled[i])
                continue;

            const auto& monitor = state.monitors[i];
            minX = min(minX, monitor.x);
            minY = min(minY, monitor.y);
            maxX = max(maxX, monitor.x + monitor.width);
            maxY = max(maxY, monitor.y + monitor.height);
            ++assignedCount;
        }

        if (assignedCount > 0)
        {
            setWindowPos = true;
            wx = minX;
            wy = minY;
            ww = maxX - minX;
            wh = maxY - minY;

            // A single monitor means "stay exactly here"; several means "stay within these"
            lockWindow = (assignedCount == 1);
        }
    }

    ConfigureInstance(instanceHandle, instance, profile, 1, setWindowPos, wx, wy, ww, wh);

    SetExternalFreezeFakeInput(instanceHandle, cfg->freezeInputUntilStart);

    // Bind every device that is currently assigned to any running app
    RebindAllInputDevices();

    if (cfg->lockRealInput && !isInputCurrentlyLocked)
    {
        LockInput(true);
        SuspendExplorer();
        isInputCurrentlyLocked = true;
    }

    // Remember the window target so we can keep the app on its display(s)
    cfg->windowLockEnabled = setWindowPos;
    cfg->lockWindowStrict = lockWindow;
    cfg->lockX = wx;
    cfg->lockY = wy;
    cfg->lockWidth = ww;
    cfg->lockHeight = wh;
    cfg->targetHwnd = nullptr;
    cfg->targetPid = instance.runtime ? instance.pid : pid;

    // Watch the target process so we can release devices the moment it exits
    cfg->targetProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, cfg->targetPid);

    // Route this app's audio to the chosen output device(s)
    StartInstanceAudio(cfg->id);

    instance.hasBeenInjected = true;

    cfg->hasInjected = true;
    cfg->running = true;
    if (cfg->name == L"New app" || cfg->name.empty())
    {
        cfg->name = cfg->launchNewInstance
            ? std::filesystem::path(cfg->gameFilepath).filename().wstring()
            : cfg->runningProcessName;
    }
    cfg->statusMessage = "Running on the selected display.";

    return true;
}

// Finds the main visible window of a process so we can re-apply the display lock
static HWND FindMainWindowForPid(unsigned long pid)
{
    HWND found = nullptr;

    struct FindState { unsigned long pid; HWND hwnd; };
    FindState findState{ pid, nullptr };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
    {
        auto& find = *reinterpret_cast<FindState*>(lParam);

        DWORD windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);

        if (windowPid != find.pid || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr)
            return TRUE;

        if (GetWindowTextLengthW(hwnd) == 0)
            return TRUE;

        find.hwnd = hwnd;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&findState));

    found = findState.hwnd;
    return found;
}

// Periodically force each running instance's window back onto its chosen display.
static void ApplyWindowLock()
{
    auto& state = GetAppState();

    for (auto& cfg : state.instances)
    {
        if (!cfg.hasInjected || !cfg.windowLockEnabled)
            continue;

        if (cfg.targetHwnd == nullptr || !IsWindow(cfg.targetHwnd))
        {
            if (cfg.targetPid != 0)
                cfg.targetHwnd = FindMainWindowForPid(cfg.targetPid);
        }

        if (cfg.targetHwnd == nullptr || !IsWindow(cfg.targetHwnd))
            continue;

        RECT current{};
        if (!GetWindowRect(cfg.targetHwnd, &current))
            continue;

        const int targetW = cfg.lockWidth;
        const int targetH = cfg.lockHeight;

        if (cfg.lockWindowStrict)
        {
            // One monitor assigned: the window must sit exactly on it.
            const bool offTarget =
                current.left != cfg.lockX ||
                current.top != cfg.lockY ||
                (current.right - current.left) != targetW ||
                (current.bottom - current.top) != targetH;

            if (offTarget)
            {
                SetWindowPos(cfg.targetHwnd, nullptr, cfg.lockX, cfg.lockY, targetW, targetH,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        else
        {
            // Several monitors assigned: the window may be moved/resized by the app, but
            // it must stay inside the combined area so it never lands on a screen you did
            // not assign. We keep the app's own size and only nudge position back inside.
            const int winW = current.right - current.left;
            const int winH = current.bottom - current.top;

            int newX = current.left;
            int newY = current.top;

            if (current.left < cfg.lockX)                 newX = cfg.lockX;
            if (current.top < cfg.lockY)                  newY = cfg.lockY;
            if (current.right > cfg.lockX + targetW)      newX = cfg.lockX + targetW - winW;
            if (current.bottom > cfg.lockY + targetH)     newY = cfg.lockY + targetH - winH;

            if (newX != current.left || newY != current.top)
            {
                SetWindowPos(cfg.targetHwnd, nullptr, newX, newY, 0, 0,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
            }
        }
    }
}

// Watches running instances and stops any whose target process has exited or crashed.
// This is what releases the bound devices automatically when an app is closed from the
// taskbar, killed, or crashes, so input is not left swallowed system-wide.
static void CheckForExitedProcesses()
{
    auto& state = GetAppState();

    std::vector<int> exited;

    for (const auto& cfg : state.instances)
    {
        if (!cfg.hasInjected || cfg.targetProcessHandle == nullptr)
            continue;

        if (WaitForSingleObject(cfg.targetProcessHandle, 0) == WAIT_OBJECT_0)
            exited.push_back(cfg.id);
    }

    for (int id : exited)
        StopInstance(id);
}

void StopInstance(int id)
{
    auto& state = GetAppState();

    for (auto& cfg : state.instances)
    {
        if (cfg.id != id)
            continue;

        StopInstanceAudio(cfg.id);

        cfg.hasInjected = false;
        cfg.running = false;
        cfg.instanceHandle = 0;
        cfg.windowLockEnabled = false;
        cfg.targetHwnd = nullptr;
        cfg.targetPid = 0;

        if (cfg.targetProcessHandle != nullptr)
        {
            CloseHandle((HANDLE)cfg.targetProcessHandle);
            cfg.targetProcessHandle = nullptr;
        }

        cfg.statusMessage = "Stopped. Start again any time.";
        break;
    }

    // If nothing is running any more, release the global locks
    bool anyRunning = false;
    for (const auto& cfg : state.instances)
        anyRunning = anyRunning || cfg.hasInjected;

    if (!anyRunning && isInputCurrentlyLocked)
    {
        LockInput(false);
        RestartExplorer();
        isInputCurrentlyLocked = false;
    }

    RebindAllInputDevices();
}

void RebindAllInputDevices()
{
    auto& state = GetAppState();

    UnbindAllInputDevices();

    for (const auto& cfg : state.instances)
    {
        if (!cfg.hasInjected)
            continue;

        for (int i = 0; i < (int)cfg.mouseEnabled.size() && i < (int)state.mice.size(); ++i)
        {
            if (cfg.mouseEnabled[i])
                BindInputDevice(state.mice[i].handle, false);
        }

        for (int i = 0; i < (int)cfg.keyboardEnabled.size() && i < (int)state.keyboards.size(); ++i)
        {
            if (cfg.keyboardEnabled[i])
                BindInputDevice(state.keyboards[i].handle, true);
        }
    }
}

void StartInstanceAudio(int id)
{
    auto& state = GetAppState();

    for (const auto& cfg : state.instances)
    {
        if (cfg.id != id)
            continue;

        if (!cfg.routeAudio || cfg.targetPid == 0)
            return;

        std::vector<std::wstring> chosen;

        for (int i = 0; i < (int)cfg.audioEnabled.size() && i < (int)state.audioOutputs.size(); ++i)
        {
            if (cfg.audioEnabled[i])
                chosen.push_back(state.audioOutputs[i].id);
        }

        if (chosen.empty())
            return;

        StartAudioRouting(cfg.targetPid, chosen);
        return;
    }
}

void StopInstanceAudio(int id)
{
    auto& state = GetAppState();

    for (const auto& cfg : state.instances)
    {
        if (cfg.id != id)
            continue;

        if (cfg.targetPid != 0)
            StopAudioRouting(cfg.targetPid);

        return;
    }
}

// Draws a selectable list row inside a card
static bool ListRow(const char* id, const std::string& primary, const std::string& secondary, bool selected)
{
    ImGui::PushID(id);

    const float rowHeight = secondary.empty() ? 30.0f : 46.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;

    const bool clicked = ImGui::InvisibleButton("##row", ImVec2(width, rowHeight));

    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImU32 bg = selected ? IM_COL32(99, 102, 241, 60)
                        : (hovered ? IM_COL32(255, 255, 255, 12) : IM_COL32(0, 0, 0, 0));
    draw->AddRectFilled(p, ImVec2(p.x + width, p.y + rowHeight), bg, 8.0f);

    if (selected)
        draw->AddRectFilled(p, ImVec2(p.x + 3, p.y + rowHeight), ColorAccent(), 2.0f);

    const float textX = p.x + 12;
    float textY = p.y + (secondary.empty() ? 7.0f : 5.0f);
    draw->AddText(ImVec2(textX, textY), IM_COL32(238, 240, 245, 255), primary.c_str());

    if (!secondary.empty())
    {
        draw->AddText(g_FontSmall, g_FontSmall->FontSize,
                      ImVec2(textX, textY + 18), IM_COL32(148, 163, 184, 255), secondary.c_str());
    }

    ImGui::PopID();
    return clicked;
}

// A compact row for the sidebar: name + status dot. Returns true when clicked.
// Sets removeRequested=true if the user chose "Remove app" from the context menu.
static bool SidebarRow(int id, const std::string& name, bool running, bool selected, bool& removeRequested)
{
    removeRequested = false;
    ImGui::PushID(id);

    const float rowHeight = 42.0f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;

    const bool clicked = ImGui::InvisibleButton("##sbrow", ImVec2(width, rowHeight));
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 rowMax(p.x + width, p.y + rowHeight);

    ImU32 bg = selected ? IM_COL32(99, 102, 241, 70)
                        : (hovered ? IM_COL32(255, 255, 255, 14) : IM_COL32(0, 0, 0, 0));
    draw->AddRectFilled(p, rowMax, bg, 8.0f);

    if (selected)
        draw->AddRectFilled(p, ImVec2(p.x + 3, rowMax.y), ColorAccent(), 2.0f);

    // Status dot
    const ImU32 dot = running ? ColorSuccess() : IM_COL32(120, 126, 140, 255);
    draw->AddCircleFilled(ImVec2(p.x + 16, p.y + rowHeight * 0.5f), 5.0f, dot);

    draw->AddText(g_FontRegular, g_FontRegular->FontSize,
                  ImVec2(p.x + 32, p.y + 11), IM_COL32(238, 240, 245, 255), name.c_str());

    // Right-click a row to remove that app
    if (ImGui::BeginPopupContextItem("##sbctx"))
    {
        if (ImGui::MenuItem("Remove app", nullptr, false, !running))
        {
            removeRequested = true;
        }

        if (running)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
            ImGui::TextUnformatted("Stop it first");
            ImGui::PopStyleColor();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    return clicked;
}

// Human-readable device summary for an instance
static std::string DescribeDevices(const InstanceConfig& cfg)
{
    auto& state = GetAppState();
    std::string out;

    for (int i = 0; i < (int)cfg.controllerEnabled.size() && i < (int)state.controllers.size(); ++i)
        if (cfg.controllerEnabled[i]) out += "controller, ";

    for (int i = 0; i < (int)cfg.mouseEnabled.size() && i < (int)state.mice.size(); ++i)
        if (cfg.mouseEnabled[i]) out += "mouse, ";

    for (int i = 0; i < (int)cfg.keyboardEnabled.size() && i < (int)state.keyboards.size(); ++i)
        if (cfg.keyboardEnabled[i]) out += "keyboard, ";

    if (out.empty())
        return "No devices assigned";

    out.erase(out.size() - 2);
    return out;
}

static std::string DescribeDisplay(const InstanceConfig& cfg)
{
    auto& state = GetAppState();

    if (!cfg.moveWindowToDisplay)
        return "Stays where it opens";

    std::string out;
    int count = 0;

    for (int i = 0; i < (int)cfg.monitorEnabled.size() && i < (int)state.monitors.size(); ++i)
    {
        if (!cfg.monitorEnabled[i])
            continue;

        if (count > 0)
            out += " + ";

        out += MonitorLabel(state.monitors[i]);
        ++count;
    }

    if (count == 0)
        return "Display";

    if (count > 1)
        out += "  (window may move between them)";

    return out;
}

static std::string DescribeAudio(const InstanceConfig& cfg)
{
    auto& state = GetAppState();
    std::string out;

    for (int i = 0; i < (int)cfg.audioEnabled.size() && i < (int)state.audioOutputs.size(); ++i)
    {
        if (!cfg.audioEnabled[i])
            continue;

        if (!out.empty())
            out += " + ";

        out += utf8_encode(state.audioOutputs[i].name);
    }

    if (out.empty())
        return "Normal system audio";

    return out;
}

// ---------------- Configure pane (per selected instance) ----------------
static void RenderConfigurePane()
{
    auto& state = GetAppState();
    auto* cfgPtr = GetSelectedInstance();

    if (cfgPtr == nullptr)
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
        ImGui::TextWrapped("No app selected. Click \"+ Add app\" in the sidebar to set one up.");
        ImGui::PopStyleColor();
        return;
    }

    auto& cfg = *cfgPtr;
    const float fullWidth = ImGui::GetContentRegionAvail().x - 8;

    // Name + status header
    {
        char nameBuf[128];
        strncpy_s(nameBuf, utf8_encode(cfg.name).c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';

        ImGui::SetNextItemWidth(320);
        if (ImGui::InputText("##instname", nameBuf, sizeof(nameBuf)))
            cfg.name = utf8_decode(nameBuf);

        ImGui::SameLine();
        if (cfg.hasInjected)
            StatusPill("RUNNING", ColorSuccess());
        else
            StatusPill("NOT RUNNING", ColorMuted());

        // Remove this app from the list
        ImGui::SameLine();

        const bool isRunning = cfg.hasInjected;
        if (isRunning) PushDisabledLocal();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.14f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.18f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.20f, 0.22f, 1.0f));
        if (ImGui::Button("Remove", ImVec2(110, 0)))
        {
            RemoveInstance(cfg.id);
            ImGui::PopStyleColor(3);
            if (isRunning) PopDisabledLocal();
            return;
        }
        ImGui::PopStyleColor(3);
        if (isRunning) PopDisabledLocal();

        if (isRunning)
        {
            ImGui::SameLine();
            ImGui::PushFont(g_FontSmall);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
            ImGui::TextUnformatted("(stop it first to remove)");
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }
    }

    ImGui::Spacing();

    // ===== Step 1: App =====
    if (CardBegin("##card_game", ImVec2(fullWidth, 0), "1. Choose the app or game",
                  "Launch something new, or attach to an app that is already running."))
    {
        if (ImGui::RadioButton("Launch a new app##src", cfg.launchNewInstance))
            cfg.launchNewInstance = true;
        ImGui::SameLine();
        if (ImGui::RadioButton("Use a running app##src", !cfg.launchNewInstance))
            cfg.launchNewInstance = false;

        ImGui::Spacing();

        if (cfg.launchNewInstance)
        {
            if (ImGui::Button("Browse for an .exe", ImVec2(220, 0)))
            {
                wchar_t szFile[260]{};
                OPENFILENAMEW ofn{};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = splitPlayHostHwnd;
                ofn.lpstrFile = szFile;
                ofn.lpstrFile[0] = '\0';
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = L"Executable\0*.EXE\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileNameW(&ofn) == TRUE)
                    cfg.gameFilepath = szFile;
            }

            ImGui::SameLine();
            if (cfg.gameFilepath.empty())
                ImGui::TextDisabled("Nothing selected yet");
            else
                ImGui::TextWrapped("%ws", std::filesystem::path(cfg.gameFilepath).filename().c_str());
        }
        else
        {
            ImGui::TextDisabled("Pick from the apps currently running:");
            ImGui::Spacing();

            static char searchBuf[128] = "";
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##search", "Search running apps...", searchBuf, sizeof(searchBuf));

            ImGui::Spacing();

            ImGui::BeginChild("##proc_list", ImVec2(0, 200), true);
            const std::string search = searchBuf;
            bool any = false;
            for (int i = 0; i < (int)state.runningGames.size(); ++i)
            {
                const auto& proc = state.runningGames[i];
                const auto name = utf8_encode(proc.name);
                const auto title = utf8_encode(proc.windowTitle);

                if (!search.empty() && name.find(search) == std::string::npos &&
                    title.find(search) == std::string::npos)
                    continue;

                any = true;
                const bool selected = (cfg.runningPid == proc.pid);
                if (ListRow(("proc" + std::to_string(i)).c_str(), name, title, selected))
                {
                    cfg.runningPid = proc.pid;
                    cfg.runningProcessName = proc.name;
                }
            }

            if (!any)
                ImGui::TextDisabled("No running apps with a window found");

            ImGui::EndChild();

            ImGui::Spacing();
            if (cfg.runningPid != 0)
                ImGui::Text("Selected: %ws (PID %lu)", cfg.runningProcessName.c_str(), cfg.runningPid);
        }
    }
    CardEnd();

    ImGui::Spacing();

    // ===== Step 2: Display =====
    if (CardBegin("##card_display", ImVec2(fullWidth, 0), "2. Choose the display(s)",
                  "Pick one display to lock the window there, or several to let it move between them."))
    {
        if (state.monitors.empty())
        {
            ImGui::TextDisabled("No displays detected");
        }
        else
        {
            for (int i = 0; i < (int)state.monitors.size(); ++i)
            {
                const auto& monitor = state.monitors[i];
                auto primary = MonitorLabel(monitor);
                std::wstring secondary = monitor.isPrimary ? L"Primary display" : L"Secondary display";
                const bool active = i < (int)cfg.monitorEnabled.size() && cfg.monitorEnabled[i];

                if (DeviceRow(("mon" + std::to_string(i)).c_str(), DeviceIcon::Display,
                              primary, utf8_encode(secondary), active, active))
                    ToggleMonitor(cfg.id, i);
            }

            const int count = CountAssignedMonitors(cfg);
            ImGui::PushFont(g_FontSmall);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.62f, 0.70f, 1.0f));
            if (count <= 1)
                ImGui::TextWrapped("One display selected: the window will be locked to it and cannot be moved.");
            else
                ImGui::TextWrapped("%d displays selected: the window may be moved between them, but not onto any other screen.", count);
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }

        ImGui::Spacing();
        Toggle("Move the window to the selected display(s)", &cfg.moveWindowToDisplay);
    }
    CardEnd();

    ImGui::Spacing();

    // ===== Step 2b: Audio output(s) =====
    if (CardBegin("##card_audio", ImVec2(fullWidth, 0), "2b. Choose audio output(s)",
                  "Send this app's sound to the output device(s) you pick. Leave all OFF to keep normal system audio."))
    {
        if (state.audioOutputs.empty())
        {
            ImGui::TextDisabled("No audio output devices detected");
        }
        else
        {
            for (int i = 0; i < (int)state.audioOutputs.size(); ++i)
            {
                const auto& out = state.audioOutputs[i];
                const bool active = i < (int)cfg.audioEnabled.size() && cfg.audioEnabled[i];

                std::wstring secondary = out.isDefault ? L"Default output" : L"Audio output";
                if (!out.description.empty() && out.description != out.name)
                    secondary += L" - " + out.description;

                if (DeviceRow(("audio" + std::to_string(i)).c_str(), DeviceIcon::Speaker,
                              utf8_encode(out.name), utf8_encode(secondary), active, active))
                    ToggleAudioOutput(cfg.id, i);
            }

            ImGui::PushFont(g_FontSmall);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.62f, 0.70f, 1.0f));
            ImGui::TextWrapped("Route-only: the app's sound is sent to the chosen device(s). Other apps are not blocked "
                               "from those devices (Windows has no way to do that without a custom audio driver).");
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }
    }
    CardEnd();

    ImGui::Spacing();

    // ===== Step 3: Devices =====
    if (CardBegin("##card_devices", ImVec2(fullWidth, 0), "3. Assign devices",
                  "A device can belong to only one app. Turning it ON here removes it from every other app."))
    {
        // Identify helper: move or click a mouse/keyboard and its row lights up green
        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.53f, 0.87f, 0.63f, 1.0f));
        ImGui::TextWrapped("Tip: move a mouse or press a key - the matching device lights up green so you can tell which is which.");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Spacing();

        // ---- Controllers ----
        ImGui::PushFont(g_FontBold);
        ImGui::TextUnformatted("Controllers");
        ImGui::PopFont();
        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.62f, 0.70f, 1.0f));
        ImGui::TextWrapped("Pick the controller this app should listen to.");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Spacing();

        if (state.controllers.empty())
            ImGui::TextDisabled("No controller detected - connect one and press Refresh devices");
        else
        {
            for (int i = 0; i < (int)state.controllers.size(); ++i)
            {
                const auto& c = state.controllers[i];
                const bool selected = cfg.selectedControllerIndex == i;
                const bool active = i < (int)cfg.controllerEnabled.size() && cfg.controllerEnabled[i];
                const char* api = c.api == ControllerApi::XInput ? "Xbox / XInput controller" :
                                  c.api == ControllerApi::OpenXInput ? "XInput controller (OpenXinput)" :
                                  "DirectInput controller";

                const bool glowing = c.api == ControllerApi::XInput && IsControllerActive(c.index);

                if (DeviceRow(("ctrl" + std::to_string(i)).c_str(), DeviceIcon::Controller,
                              utf8_encode(c.name), api, active, selected, glowing))
                {
                    if (selected && active)
                        AssignController(cfg.id, i, false);
                    else
                        AssignController(cfg.id, i, true);
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Mice ----
        ImGui::PushFont(g_FontBold);
        ImGui::TextUnformatted("Mice");
        ImGui::PopFont();
        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.62f, 0.70f, 1.0f));
        ImGui::TextWrapped("Optional. Leave all OFF to keep your mouse on the desktop.");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Spacing();

        if (state.mice.empty())
            ImGui::TextDisabled("No mice detected");
        else
        {
            for (int i = 0; i < (int)state.mice.size(); ++i)
            {
                const bool selected = cfg.selectedMouseIndex == i;
                const bool active = i < (int)cfg.mouseEnabled.size() && cfg.mouseEnabled[i];

                if (DeviceRow(("mouse" + std::to_string(i)).c_str(), DeviceIcon::Mouse,
                              utf8_encode(state.mice[i].name), "Mouse", active, selected,
                              IsDeviceActive(state.mice[i].handle)))
                {
                    if (selected && active)
                        AssignMouse(cfg.id, i, false);
                    else
                        AssignMouse(cfg.id, i, true);
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Keyboards ----
        ImGui::PushFont(g_FontBold);
        ImGui::TextUnformatted("Keyboards");
        ImGui::PopFont();
        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.62f, 0.70f, 1.0f));
        ImGui::TextWrapped("Optional. Leave all OFF to keep your keyboard on the desktop.");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Spacing();

        if (state.keyboards.empty())
            ImGui::TextDisabled("No keyboards detected");
        else
        {
            for (int i = 0; i < (int)state.keyboards.size(); ++i)
            {
                const bool selected = cfg.selectedKeyboardIndex == i;
                const bool active = i < (int)cfg.keyboardEnabled.size() && cfg.keyboardEnabled[i];

                if (DeviceRow(("kb" + std::to_string(i)).c_str(), DeviceIcon::Keyboard,
                              utf8_encode(state.keyboards[i].name), "Keyboard", active, selected,
                              IsDeviceActive(state.keyboards[i].handle)))
                {
                    if (selected && active)
                        AssignKeyboard(cfg.id, i, false);
                    else
                        AssignKeyboard(cfg.id, i, true);
                }
            }
        }

        ImGui::Spacing();
        if (ImGui::Button("Refresh devices", ImVec2(220, 0)))
            RefreshSimpleModeDevices();
    }
    CardEnd();

    ImGui::Spacing();

    // ===== Step 4: Options + Start =====
    if (CardBegin("##card_start", ImVec2(fullWidth, 0), "4. Start", nullptr))
    {
        Toggle("Show in-game mouse cursor", &cfg.showFakeCursor);
        Toggle("Freeze app input until unlocked (End key)", &cfg.freezeInputUntilStart);
        Toggle("Lock the real keyboard/mouse while playing", &cfg.lockRealInput);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!cfg.hasInjected)
        {
            const bool canStart = (cfg.launchNewInstance ? !cfg.gameFilepath.empty() : cfg.runningPid != 0);

            PushAccentButton(true);
            if (!canStart) PushDisabledLocal();
            if (ImGui::Button("Start this app", ImVec2(-1, 46)))
                StartInstance(cfg.id);
            if (!canStart) PopDisabledLocal();
            PopAccentButton();

            if (!canStart)
            {
                ImGui::PushFont(g_FontSmall);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
                ImGui::TextWrapped("Select an app or game in step 1 first.");
                ImGui::PopStyleColor();
                ImGui::PopFont();
            }
        }
        else
        {
            PushAccentButton(false);
            if (ImGui::Button("Stop this app", ImVec2(-1, 46)))
                StopInstance(cfg.id);
            PopAccentButton();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
            ImGui::TextWrapped("%s", cfg.statusMessage.c_str());
            ImGui::PopStyleColor();
        }
    }
    CardEnd();
}

// ---------------- Running pane (all instances) ----------------
static void RenderRunningPane()
{
    auto& state = GetAppState();
    const float fullWidth = ImGui::GetContentRegionAvail().x - 8;

    if (state.instances.empty())
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
        ImGui::TextWrapped("Nothing added yet. Use \"+ Add app\" in the sidebar.");
        ImGui::PopStyleColor();
        return;
    }

    for (auto& cfg : state.instances)
    {
        const std::string cardId = "##run" + std::to_string(cfg.id);

        if (CardBegin(cardId.c_str(), ImVec2(fullWidth, 0), utf8_encode(cfg.name).c_str(),
                      cfg.hasInjected ? "Running" : "Not running"))
        {
            ImGui::PushFont(g_FontSmall);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
            ImGui::Text("Display: %s", DescribeDisplay(cfg).c_str());
            ImGui::Text("Audio: %s", DescribeAudio(cfg).c_str());
            ImGui::Text("Devices: %s", DescribeDevices(cfg).c_str());
            ImGui::PopStyleColor();

            if (cfg.hasInjected)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.53f, 0.87f, 0.63f, 1.0f));
                ImGui::Text("Injected instance #%u", cfg.instanceHandle);
                ImGui::PopStyleColor();
            }

            ImGui::PopFont();

            ImGui::Spacing();

            ImGui::PushID(cfg.id);

            if (cfg.hasInjected)
            {
                PushAccentButton(false);
                if (ImGui::Button("Stop", ImVec2(160, 34)))
                    StopInstance(cfg.id);
                PopAccentButton();

                ImGui::SameLine();
                if (ImGui::Button("Select", ImVec2(120, 34)))
                    state.selectedInstanceId = cfg.id;
            }
            else
            {
                PushAccentButton(true);
                if (ImGui::Button("Start", ImVec2(160, 34)))
                    StartInstance(cfg.id);
                PopAccentButton();

                ImGui::SameLine();
                if (ImGui::Button("Configure", ImVec2(120, 34)))
                    state.selectedInstanceId = cfg.id;
            }

            ImGui::PopID();
        }
        CardEnd();

        ImGui::Spacing();
    }
}

void RenderSimpleMode()
{
    auto& state = GetAppState();

    // Controllers aren't raw input, so poll them each frame to light up the active pad
    PollControllerActivity();

    // Keep every app window locked to its assigned display
    ApplyWindowLock();

    // Release devices/audio and stop instances whose target process has exited or crashed
    CheckForExitedProcesses();

    // Keep process list fresh-ish
    static int frameCounter = 0;
    if (frameCounter++ % 120 == 0)
        RefreshSimpleModeProcesses();

    const float availHeight = ImGui::GetContentRegionAvail().y;

    // ---------------- Left sidebar ----------------
    ImGui::BeginChild("##sidebar", ImVec2(260, availHeight), true);

    ImGui::PushFont(g_FontBold);
    ImGui::TextUnformatted("Apps");
    ImGui::PopFont();

    ImGui::Spacing();

    PushAccentButton(true);
    if (ImGui::Button("+ Add app", ImVec2(-1, 36)))
        AddInstance();
    PopAccentButton();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    int removeId = -1;

    for (auto& cfg : state.instances)
    {
        const bool selected = (state.selectedInstanceId == cfg.id);
        bool removeRequested = false;

        if (SidebarRow(cfg.id, utf8_encode(cfg.name), cfg.hasInjected, selected, removeRequested))
            state.selectedInstanceId = cfg.id;

        if (removeRequested)
            removeId = cfg.id;
    }

    if (removeId != -1)
        RemoveInstance(removeId);

    ImGui::Spacing();

    // Footer: counts
    {
        int running = 0;
        for (const auto& cfg : state.instances)
            running += cfg.hasInjected ? 1 : 0;

        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.54f, 0.62f, 1.0f));
        ImGui::Text("%d added, %d running", (int)state.instances.size(), running);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // ---------------- Main area ----------------
    ImGui::BeginChild("##main", ImVec2(0, availHeight), false);

    static int activeTab = 0;
    (void)activeTab;

    if (ImGui::BeginTabBar("##tabs"))
    {
        if (ImGui::BeginTabItem("Setup"))
        {
            activeTab = 0;
            ImGui::Spacing();
            RenderConfigurePane();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Running"))
        {
            activeTab = 1;
            ImGui::Spacing();
            RenderRunningPane();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::EndChild();
}

}
