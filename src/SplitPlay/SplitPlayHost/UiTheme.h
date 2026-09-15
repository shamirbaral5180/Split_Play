#pragma once
#include <imgui.h>
#include <string>

namespace SplitPlayHost
{

// Fonts loaded at startup (may be null if loading failed)
extern ImFont* g_FontRegular;
extern ImFont* g_FontBold;
extern ImFont* g_FontSmall;

// Applies the modern dashboard theme to ImGui
void ApplyModernStyle();

// Loads modern fonts (Segoe UI if available, otherwise falls back)
void LoadModernFonts(ImGuiIO& io);

// Modern widget helpers
bool Toggle(const char* label, bool* value);
void PushAccentButton(bool primary);
void PopAccentButton();

// A rounded card container. Use CardBegin/CardEnd around content.
bool CardBegin(const char* id, const ImVec2& size, const char* title = nullptr, const char* subtitle = nullptr);
void CardEnd();

// A small coloured status pill
void StatusPill(const char* text, ImU32 color);

// Section title with an accent bar
void SectionTitle(const char* text, const char* subtitle = nullptr);

// Device type icons drawn with the ImGui draw list (no image assets needed)
enum class DeviceIcon
{
	Controller,
	Mouse,
	Keyboard,
	Display,
	Speaker,
	Microphone,
	Camera
};

// Draws an icon at the current cursor position and advances the cursor.
void DeviceIconWidget(DeviceIcon icon, float size = 34.0f);

// Draws a device icon into an explicit rectangle
void DrawDeviceIcon(ImDrawList* draw, DeviceIcon icon, const ImVec2& min, const ImVec2& max, ImU32 color);

// A full-width device row: icon, name, subtitle, and an on/off badge.
// Set `glowing` when this device is the one currently being used (it lights up).
// Set `readOnly` for identify-only rows (cameras/microphones) so the badge reads AVAILABLE.
// Returns true when clicked.
bool DeviceRow(const char* id, DeviceIcon icon, const std::string& name, const std::string& type,
               bool active, bool selected, bool glowing = false, bool readOnly = false);

// Helpers
ImU32 ColorAccent();
ImU32 ColorSuccess();
ImU32 ColorWarning();
ImU32 ColorDanger();
ImU32 ColorMuted();

}
