#include "Gui.h"
#include <imgui.h>
#include <algorithm>
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
    StartInputActivityMonitor();

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
        ImGui::BeginChild("##simple_scroll", ImVec2(0, 0), false, ImGuiWindowFlags_None);
        RenderSimpleMode();
        ImGui::EndChild();
    }
    ImGui::End();
}

bool LaunchSimple()
{
    auto& state = GetSimpleModeState();

    if (state.hasInjected)
    {
        state.statusMessage = "Already running. Stop it first.";
        return false;
    }

    // Build the instance
    Instance instance = state.launchNewInstance
        ? Instance(state.gameFilepath, std::filesystem::path(state.gameFilepath).filename().wstring())
        : Instance(state.runningPid, state.runningProcessName);

    // Profile: second-screen controller preset, then apply the user's device assignments
    auto profile = Profile::MakeSecondScreenControllerProfile();

    if (state.selectedControllerIndex >= 0 && state.selectedControllerIndex < (int)state.controllers.size() &&
        state.selectedControllerIndex < (int)state.controllerEnabled.size() &&
        state.controllerEnabled[state.selectedControllerIndex])
    {
        const auto& controller = state.controllers[state.selectedControllerIndex];
        instance.controllerIndex = controller.index;
        profile.dinputToXinputRedirection = controller.requiresDinputRedirection;
        profile.useOpenXinput = controller.requiresOpenXinput;
    }
    else
    {
        instance.controllerIndex = 0;
    }

    // Mouse / keyboard assigned to the game (optional)
    if (state.selectedMouseIndex >= 0 && state.selectedMouseIndex < (int)state.mice.size() &&
        state.selectedMouseIndex < (int)state.mouseEnabled.size() &&
        state.mouseEnabled[state.selectedMouseIndex])
    {
        instance.mouseHandle = state.mice[state.selectedMouseIndex].handle;
    }
    else
    {
        instance.mouseHandle = -1;
    }

    if (state.selectedKeyboardIndex >= 0 && state.selectedKeyboardIndex < (int)state.keyboards.size() &&
        state.selectedKeyboardIndex < (int)state.keyboardEnabled.size() &&
        state.keyboardEnabled[state.selectedKeyboardIndex])
    {
        instance.keyboardHandle = state.keyboards[state.selectedKeyboardIndex].handle;
    }
    else
    {
        instance.keyboardHandle = -1;
    }

    profile.drawFakeMouseCursor = state.showFakeCursor;

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
        state.statusMessage = "Injection failed. See console for details.";
        return false;
    }

    trackedInstanceHandles.push_back(instanceHandle);
    state.instanceHandle = instanceHandle;

    // Window placement on the chosen monitor
    bool setWindowPos = false;
    int wx = 0, wy = 0, ww = 0, wh = 0;

    if (state.moveWindowToDisplay && state.selectedMonitorIndex >= 0 && state.selectedMonitorIndex < (int)state.monitors.size())
    {
        const auto& monitor = state.monitors[state.selectedMonitorIndex];
        setWindowPos = true;
        wx = monitor.x;
        wy = monitor.y;
        ww = monitor.width;
        wh = monitor.height;
    }

    ConfigureInstance(instanceHandle, instance, profile, 1, setWindowPos, wx, wy, ww, wh);

    SetExternalFreezeFakeInput(instanceHandle, state.freezeInputUntilStart);

    if (state.lockRealInput)
    {
        LockInput(true);
        SuspendExplorer();
        isInputCurrentlyLocked = true;
    }

    instance.hasBeenInjected = true;

    state.hasInjected = true;
    state.running = true;
    state.statusMessage = "Running. The app is on the selected display.";

    return true;
}

static void StopSimple()
{
    auto& state = GetSimpleModeState();

    if (isInputCurrentlyLocked)
    {
        LockInput(false);
        RestartExplorer();
        isInputCurrentlyLocked = false;
    }

    state.hasInjected = false;
    state.running = false;
    state.instanceHandle = 0;
    state.statusMessage = "Stopped. You can start again (restart the app if needed).";
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

void RenderSimpleMode()
{
    auto& state = GetSimpleModeState();

    // Keep process list fresh-ish
    static int frameCounter = 0;
    if (frameCounter++ % 120 == 0)
        RefreshSimpleModeProcesses();

    const float fullWidth = ImGui::GetContentRegionAvail().x;

    // ---------------- Header ----------------
    ImGui::PushFont(g_FontBold);
    ImGui::TextUnformatted("SplitPlay Dashboard");
    ImGui::PopFont();

    ImGui::SameLine();
    ImGui::PushFont(g_FontSmall);
    if (state.hasInjected)
        StatusPill("RUNNING", ColorSuccess());
    else
        StatusPill("READY", ColorAccent());
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
    ImGui::TextWrapped("Send an app or game to your second screen and control it with the devices you choose, "
                       "while your keyboard and mouse keep working on the main screen.");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ===== Step 1: Game =====
    if (CardBegin("##card_game", ImVec2(fullWidth, 0), "1. Choose the app or game",
                  "Launch something new, or attach to an app that is already running."))
    {
        if (ImGui::RadioButton("Launch a new app##src", state.launchNewInstance))
            state.launchNewInstance = true;
        ImGui::SameLine();
        if (ImGui::RadioButton("Use a running app##src", !state.launchNewInstance))
            state.launchNewInstance = false;

        ImGui::Spacing();

        if (state.launchNewInstance)
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
                    state.gameFilepath = szFile;
            }

            ImGui::SameLine();
            if (state.gameFilepath.empty())
                ImGui::TextDisabled("Nothing selected yet");
            else
                ImGui::TextWrapped("%ws", std::filesystem::path(state.gameFilepath).filename().c_str());
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
                const bool selected = (state.runningPid == proc.pid);
                if (ListRow(("proc" + std::to_string(i)).c_str(), name, title, selected))
                {
                    state.runningPid = proc.pid;
                    state.runningProcessName = proc.name;
                }
            }

            if (!any)
                ImGui::TextDisabled("No running apps with a window found");

            ImGui::EndChild();

            ImGui::Spacing();
            if (state.runningPid != 0)
                ImGui::Text("Selected: %ws (PID %lu)", state.runningProcessName.c_str(), state.runningPid);
        }
    }
    CardEnd();

    ImGui::Spacing();

    // ===== Step 2: Display =====
    if (CardBegin("##card_display", ImVec2(fullWidth, 0), "2. Choose the display",
                  "Where should the window appear?"))
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
                const bool selected = state.selectedMonitorIndex == i;

                if (DeviceRow(("mon" + std::to_string(i)).c_str(), DeviceIcon::Display,
                              primary, utf8_encode(secondary), true, selected))
                    state.selectedMonitorIndex = i;
            }
        }

        ImGui::Spacing();
        Toggle("Move the window to this display", &state.moveWindowToDisplay);
    }
    CardEnd();

    ImGui::Spacing();

    // ===== Step 3: Devices =====
    if (CardBegin("##card_devices", ImVec2(fullWidth, 0), "3. Assign devices",
                  "Click a device to send it to the target app. Devices that are ON control the app; everything else keeps controlling Windows."))
    {
        // Identify helper: move or click a mouse/keyboard and its row lights up green
        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.53f, 0.87f, 0.63f, 1.0f));
        ImGui::TextWrapped("Tip: move a mouse or press a key - the matching device lights up green so you can tell which is which.");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Spacing();

        auto& ctrlEnabled = state.controllerEnabled;
        auto& mouseEnabled = state.mouseEnabled;
        auto& kbEnabled = state.keyboardEnabled;

        // ---- Controllers ----
        ImGui::PushFont(g_FontBold);
        ImGui::TextUnformatted("Controllers");
        ImGui::PopFont();
        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.62f, 0.70f, 1.0f));
        ImGui::TextWrapped("Pick the controller the app should listen to.");
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
                const bool selected = state.selectedControllerIndex == i;
                const bool active = ctrlEnabled[i];
                const char* api = c.api == ControllerApi::XInput ? "Xbox / XInput controller" :
                                  c.api == ControllerApi::OpenXInput ? "XInput controller (OpenXinput)" :
                                  "DirectInput controller";

                if (DeviceRow(("ctrl" + std::to_string(i)).c_str(), DeviceIcon::Controller,
                              utf8_encode(c.name), api, active, selected))
                {
                    // Click selects and toggles it on; if it was on and you click the selected one, it turns off
                    if (selected && active)
                        ctrlEnabled[i] = false;
                    else
                    {
                        state.selectedControllerIndex = i;
                        ctrlEnabled[i] = true;
                    }
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
                const bool selected = state.selectedMouseIndex == i;
                const bool active = mouseEnabled[i];

                if (DeviceRow(("mouse" + std::to_string(i)).c_str(), DeviceIcon::Mouse,
                              utf8_encode(state.mice[i].name), "Mouse", active, selected,
                              IsDeviceActive(state.mice[i].handle)))
                {
                    if (selected && active)
                        mouseEnabled[i] = false;
                    else
                    {
                        state.selectedMouseIndex = i;
                        mouseEnabled[i] = true;
                    }
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
                const bool selected = state.selectedKeyboardIndex == i;
                const bool active = kbEnabled[i];

                if (DeviceRow(("kb" + std::to_string(i)).c_str(), DeviceIcon::Keyboard,
                              utf8_encode(state.keyboards[i].name), "Keyboard", active, selected,
                              IsDeviceActive(state.keyboards[i].handle)))
                {
                    if (selected && active)
                        kbEnabled[i] = false;
                    else
                    {
                        state.selectedKeyboardIndex = i;
                        kbEnabled[i] = true;
                    }
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
        Toggle("Show in-game mouse cursor", &state.showFakeCursor);
        Toggle("Freeze app input until unlocked (Home key)", &state.freezeInputUntilStart);
        Toggle("Lock the real keyboard/mouse while playing", &state.lockRealInput);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!state.hasInjected)
        {
            const bool canStart = (state.launchNewInstance ? !state.gameFilepath.empty() : state.runningPid != 0);

            PushAccentButton(true);
            if (!canStart) PushDisabledLocal();
            if (ImGui::Button("Start", ImVec2(-1, 46)))
                LaunchSimple();
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
            if (ImGui::Button("Stop", ImVec2(-1, 46)))
                StopSimple();
            PopAccentButton();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.66f, 0.74f, 1.0f));
            ImGui::TextWrapped("%s", state.statusMessage.c_str());
            ImGui::PopStyleColor();
        }
    }
    CardEnd();

    ImGui::Spacing();
    ImGui::PushFont(g_FontSmall);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.54f, 0.62f, 1.0f));
    ImGui::TextWrapped("Tip: run fullscreen games in windowed or borderless mode. Exclusive fullscreen can ignore being moved "
                       "to another display.");
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

}
