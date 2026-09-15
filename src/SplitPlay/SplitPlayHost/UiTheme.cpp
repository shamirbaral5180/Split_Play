#include "UiTheme.h"
#include <windows.h>
#include <string>

namespace SplitPlayHost
{

ImFont* g_FontRegular = nullptr;
ImFont* g_FontBold = nullptr;
ImFont* g_FontSmall = nullptr;

ImU32 ColorAccent()  { return IM_COL32(99, 102, 241, 255); }   // indigo
ImU32 ColorSuccess() { return IM_COL32(34, 197, 94, 255); }    // green
ImU32 ColorWarning() { return IM_COL32(245, 158, 11, 255); }   // amber
ImU32 ColorDanger()  { return IM_COL32(239, 68, 68, 255); }    // red
ImU32 ColorMuted()   { return IM_COL32(148, 163, 184, 255); }  // slate

static const ImVec4 kAccent     = ImVec4(0.39f, 0.40f, 0.95f, 1.00f);
static const ImVec4 kAccentHov  = ImVec4(0.49f, 0.50f, 0.98f, 1.00f);
static const ImVec4 kAccentAct  = ImVec4(0.32f, 0.33f, 0.86f, 1.00f);
static const ImVec4 kBg         = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
static const ImVec4 kPanel      = ImVec4(0.11f, 0.12f, 0.16f, 1.00f);
static const ImVec4 kPanelHov   = ImVec4(0.14f, 0.15f, 0.20f, 1.00f);
static const ImVec4 kBorder     = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
static const ImVec4 kText       = ImVec4(0.93f, 0.94f, 0.97f, 1.00f);
static const ImVec4 kTextMuted  = ImVec4(0.58f, 0.62f, 0.70f, 1.00f);

void ApplyModernStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 12.0f;
    style.FrameRounding     = 8.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 8.0f;
    style.TabRounding       = 8.0f;

    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(18, 18);
    style.FramePadding      = ImVec2(12, 8);
    style.ItemSpacing       = ImVec2(10, 10);
    style.ItemInnerSpacing  = ImVec2(8, 6);
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 12.0f;

    style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
    style.ButtonTextAlign   = ImVec2(0.5f, 0.5f);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]                  = kText;
    c[ImGuiCol_TextDisabled]          = kTextMuted;
    c[ImGuiCol_WindowBg]              = kBg;
    c[ImGuiCol_ChildBg]               = kPanel;
    c[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.11f, 0.15f, 0.98f);
    c[ImGuiCol_Border]                = kBorder;
    c[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]               = ImVec4(0.15f, 0.16f, 0.21f, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.24f, 0.26f, 0.33f, 1.00f);
    c[ImGuiCol_TitleBg]               = kBg;
    c[ImGuiCol_TitleBgActive]         = kBg;
    c[ImGuiCol_TitleBgCollapsed]      = kBg;
    c[ImGuiCol_MenuBarBg]             = kPanel;
    c[ImGuiCol_ScrollbarBg]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.25f, 0.27f, 0.34f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.32f, 0.34f, 0.42f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]   = kAccent;
    c[ImGuiCol_CheckMark]             = kAccent;
    c[ImGuiCol_SliderGrab]            = kAccent;
    c[ImGuiCol_SliderGrabActive]      = kAccentAct;
    c[ImGuiCol_Button]                = ImVec4(0.18f, 0.19f, 0.25f, 1.00f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.25f, 0.32f, 1.00f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.28f, 0.30f, 0.38f, 1.00f);
    c[ImGuiCol_Header]                = ImVec4(0.18f, 0.19f, 0.25f, 1.00f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.25f, 0.32f, 1.00f);
    c[ImGuiCol_HeaderActive]          = kAccent;
    c[ImGuiCol_Separator]             = kBorder;
    c[ImGuiCol_SeparatorHovered]      = kAccentHov;
    c[ImGuiCol_SeparatorActive]       = kAccent;
    c[ImGuiCol_ResizeGrip]            = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ResizeGripHovered]     = kAccentHov;
    c[ImGuiCol_ResizeGripActive]      = kAccent;
    c[ImGuiCol_Tab]                   = ImVec4(0.11f, 0.12f, 0.16f, 1.00f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.20f, 0.21f, 0.28f, 1.00f);
    c[ImGuiCol_TabActive]             = kAccent;
    c[ImGuiCol_TabUnfocused]          = ImVec4(0.11f, 0.12f, 0.16f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.19f, 0.25f, 1.00f);
    c[ImGuiCol_PlotLines]             = kAccent;
    c[ImGuiCol_PlotHistogram]         = kAccent;
    c[ImGuiCol_TextSelectedBg]        = ImVec4(0.39f, 0.40f, 0.95f, 0.35f);
    c[ImGuiCol_NavHighlight]          = kAccent;
}

// Try to find Segoe UI on the system; fall back to any available UI font
static bool LoadFontFromFile(ImGuiIO& io, const char* path, float size, ImFont** out)
{
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
        return false;

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 1;
    config.PixelSnapH = false;

    *out = io.Fonts->AddFontFromFileTTF(path, size, &config);
    return *out != nullptr;
}

void LoadModernFonts(ImGuiIO& io)
{
    const char* regular[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\verdana.ttf",
    };
    const char* bold[] = {
        "C:\\Windows\\Fonts\\segoeuib.ttf",
        "C:\\Windows\\Fonts\\seguisb.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf",
    };

    for (const auto* path : regular)
    {
        if (LoadFontFromFile(io, path, 17.0f, &g_FontRegular))
            break;
    }

    for (const auto* path : bold)
    {
        if (LoadFontFromFile(io, path, 17.0f, &g_FontBold))
            break;
    }

    // Small font for hints/labels
    for (const auto* path : regular)
    {
        if (LoadFontFromFile(io, path, 14.0f, &g_FontSmall))
            break;
    }

    // If no TTF worked, fall back to the default font so the UI still works
    if (g_FontRegular == nullptr)
        g_FontRegular = io.Fonts->AddFontDefault();

    if (g_FontBold == nullptr)
        g_FontBold = g_FontRegular;

    if (g_FontSmall == nullptr)
        g_FontSmall = g_FontRegular;

    io.FontDefault = g_FontRegular;

    io.Fonts->Build();
}

bool Toggle(const char* label, bool* value)
{
    ImGui::PushID(label);

    const float height = ImGui::GetFrameHeight();
    const float width = height * 1.8f;
    const float radius = height * 0.5f;

    ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##toggle", ImVec2(width, height));
    if (pressed)
        *value = !*value;

    const bool hovered = ImGui::IsItemHovered();

    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImU32 bg = *value ? ColorAccent() : IM_COL32(60, 65, 80, 255);
    if (hovered)
        bg = *value ? IM_COL32(120, 122, 250, 255) : IM_COL32(75, 80, 98, 255);

    draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg, radius);

    const float knobRadius = radius - 2.0f;
    const float knobX = *value ? (p.x + width - radius) : (p.x + radius);
    const float knobY = p.y + radius;

    draw->AddCircleFilled(ImVec2(knobX, knobY), knobRadius, IM_COL32(255, 255, 255, 255));

    ImGui::SameLine();
    ImGui::TextUnformatted(label);

    ImGui::PopID();
    return pressed;
}

void PushAccentButton(bool primary)
{
    if (primary)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kAccentHov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccentAct);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.19f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.25f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.30f, 0.38f, 1.0f));
    }
}

void PopAccentButton()
{
    ImGui::PopStyleColor(3);
}

static ImVec2 g_CardStart{};
static float g_CardWidth = 0.0f;
static float g_CardPadX = 16.0f;
static float g_CardPadY = 16.0f;

bool CardBegin(const char* id, const ImVec2& size, const char* title, const char* subtitle)
{
    ImGui::PushID(id);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->ChannelsSplit(2);
    draw->ChannelsSetCurrent(1); // content is drawn above the card background

    g_CardStart = ImGui::GetCursorScreenPos();
    g_CardPadX = 16.0f;
    g_CardPadY = 16.0f;
    g_CardWidth = (size.x > 0.0f) ? size.x : ImGui::GetContentRegionAvail().x;

    ImGui::Dummy(ImVec2(0.0f, g_CardPadY * 0.6f));
    ImGui::Indent(g_CardPadX);

    if (title != nullptr)
    {
        ImGui::PushFont(g_FontBold);
        ImGui::TextUnformatted(title);
        ImGui::PopFont();

        if (subtitle != nullptr)
        {
            ImGui::PushFont(g_FontSmall);
            ImGui::PushStyleColor(ImGuiCol_Text, kTextMuted);
            ImGui::TextWrapped("%s", subtitle);
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }

        ImGui::Spacing();
    }

    return true;
}

void CardEnd()
{
    ImGui::Unindent(g_CardPadX);
    ImGui::Dummy(ImVec2(0.0f, g_CardPadY * 0.6f));

    const ImVec2 end = ImGui::GetCursorScreenPos();
    const ImVec2 cardMin = g_CardStart;
    const ImVec2 cardMax(g_CardStart.x + g_CardWidth, end.y);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->ChannelsSetCurrent(0); // background goes behind content
    draw->AddRectFilled(cardMin, cardMax, IM_COL32(18, 20, 26, 255), 12.0f);
    draw->AddRect(cardMin, cardMax, IM_COL32(44, 48, 60, 255), 12.0f, 0, 1.0f);
    draw->ChannelsMerge();

    ImGui::PopID();
}

void StatusPill(const char* text, ImU32 color)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const ImVec2 textSize = ImGui::CalcTextSize(text);
    const float padX = 10.0f;
    const float padY = 4.0f;

    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 pillMax(p.x + textSize.x + padX * 2, p.y + textSize.y + padY * 2);

    ImU32 bg = (color & 0x00FFFFFF) | 0x33000000;
    draw->AddRectFilled(p, pillMax, bg, 20.0f);
    draw->AddRect(p, pillMax, color, 20.0f, 0, 1.0f);
    draw->AddText(ImVec2(p.x + padX, p.y + padY), color, text);

    ImGui::Dummy(ImVec2(pillMax.x - p.x, pillMax.y - p.y));
}

void SectionTitle(const char* text, const char* subtitle)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float barH = ImGui::GetTextLineHeight();

    draw->AddRectFilled(p, ImVec2(p.x + 4, p.y + barH), ColorAccent(), 2.0f);
    ImGui::Dummy(ImVec2(12, 0));
    ImGui::SameLine();

    ImGui::PushFont(g_FontBold);
    ImGui::TextUnformatted(text);
    ImGui::PopFont();

    if (subtitle != nullptr)
    {
        ImGui::PushFont(g_FontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, kTextMuted);
        ImGui::TextWrapped("%s", subtitle);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
}

void DrawDeviceIcon(ImDrawList* draw, DeviceIcon icon, const ImVec2& min, const ImVec2& max, ImU32 color)
{
    const float w = max.x - min.x;
    const float h = max.y - min.y;
    const float cx = min.x + w * 0.5f;
    const float cy = min.y + h * 0.5f;
    const float thick = 2.0f;

    switch (icon)
    {
    case DeviceIcon::Mouse:
    {
        // Rounded body
        const ImVec2 a(cx - w * 0.22f, min.y + h * 0.10f);
        const ImVec2 b(cx + w * 0.22f, max.y - h * 0.10f);
        draw->AddRect(a, b, color, (b.x - a.x) * 0.5f, 0, thick);
        // Scroll wheel line
        draw->AddLine(ImVec2(cx, a.y + h * 0.06f), ImVec2(cx, cy - h * 0.02f), color, thick);
        break;
    }
    case DeviceIcon::Keyboard:
    {
        const ImVec2 a(min.x + w * 0.08f, cy - h * 0.20f);
        const ImVec2 b(max.x - w * 0.08f, cy + h * 0.20f);
        draw->AddRect(a, b, color, 3.0f, 0, thick);
        // Key dots
        for (int row = 0; row < 2; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                const float kx = a.x + (b.x - a.x) * (0.18f + col * 0.22f);
                const float ky = a.y + (b.y - a.y) * (0.30f + row * 0.40f);
                draw->AddCircleFilled(ImVec2(kx, ky), 1.3f, color);
            }
        }
        break;
    }
    case DeviceIcon::Controller:
    {
        // Body
        const ImVec2 a(min.x + w * 0.08f, cy - h * 0.20f);
        const ImVec2 b(max.x - w * 0.08f, cy + h * 0.22f);
        draw->AddRect(a, b, color, (b.y - a.y) * 0.4f, 0, thick);
        // Left stick / d-pad
        draw->AddCircle(ImVec2(a.x + (b.x - a.x) * 0.28f, cy + 0.5f), h * 0.09f, color, 12, thick);
        // Right buttons (two dots)
        draw->AddCircleFilled(ImVec2(a.x + (b.x - a.x) * 0.72f, cy - h * 0.06f), 1.8f, color);
        draw->AddCircleFilled(ImVec2(a.x + (b.x - a.x) * 0.72f, cy + h * 0.12f), 1.8f, color);
        break;
    }
    case DeviceIcon::Display:
    {
        const ImVec2 a(min.x + w * 0.10f, min.y + h * 0.16f);
        const ImVec2 b(max.x - w * 0.10f, max.y - h * 0.26f);
        draw->AddRect(a, b, color, 3.0f, 0, thick);
        // Stand
        draw->AddLine(ImVec2(cx, b.y), ImVec2(cx, max.y - h * 0.08f), color, thick);
        draw->AddLine(ImVec2(cx - w * 0.14f, max.y - h * 0.08f), ImVec2(cx + w * 0.14f, max.y - h * 0.08f), color, thick);
        break;
    }
    case DeviceIcon::Speaker:
    {
        // Speaker cone body
        const ImVec2 bodyMin(min.x + w * 0.12f, cy - h * 0.14f);
        const ImVec2 bodyMax(min.x + w * 0.34f, cy + h * 0.14f);
        draw->AddRectFilled(bodyMin, bodyMax, color, 2.0f);
        // Cone
        draw->AddTriangle(ImVec2(bodyMax.x, bodyMin.y), ImVec2(bodyMax.x, bodyMax.y),
                          ImVec2(min.x + w * 0.52f, cy + h * 0.30f), color, thick);
        draw->AddTriangle(ImVec2(bodyMax.x, bodyMin.y), ImVec2(min.x + w * 0.52f, cy - h * 0.30f),
                          ImVec2(min.x + w * 0.52f, cy + h * 0.30f), color, thick);
        // Sound waves
        draw->AddCircle(ImVec2(min.x + w * 0.62f, cy), h * 0.16f, color, 12, thick);
        draw->AddCircle(ImVec2(min.x + w * 0.62f, cy), h * 0.28f, color, 12, thick);
        break;
    }
    }
}

void DeviceIconWidget(DeviceIcon icon, float size)
{
    const ImVec2 p = ImGui::GetCursorScreenPos();
    DrawDeviceIcon(ImGui::GetWindowDrawList(), icon, p, ImVec2(p.x + size, p.y + size), ColorMuted());
    ImGui::Dummy(ImVec2(size, size));
}

bool DeviceRow(const char* id, DeviceIcon icon, const std::string& name, const std::string& type,
               bool active, bool selected, bool glowing)
{
    ImGui::PushID(id);

    const float rowHeight = 58.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;

    const bool clicked = ImGui::InvisibleButton("##devrow", ImVec2(width, rowHeight));
    const bool hovered = ImGui::IsItemHovered();

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 rowMax(p.x + width, p.y + rowHeight);

    ImU32 bg = selected ? IM_COL32(99, 102, 241, 55)
                        : (hovered ? IM_COL32(255, 255, 255, 14) : IM_COL32(24, 26, 34, 255));
    if (!active)
        bg = hovered ? IM_COL32(255, 255, 255, 10) : IM_COL32(20, 22, 28, 200);

    // When the device is being used, replace the background with a warm glow
    if (glowing)
        bg = IM_COL32(34, 197, 94, 70);

    draw->AddRectFilled(p, rowMax, bg, 10.0f);

    if (glowing)
    {
        // Outer glow ring so the name/row clearly lights up
        for (int i = 1; i <= 3; ++i)
        {
            const float off = (float)i * 1.5f;
            const ImU32 ring = ColorSuccess() & 0x00FFFFFF | ((ImU32)(70 / i) << 24);
            draw->AddRect(ImVec2(p.x - off, p.y - off), ImVec2(rowMax.x + off, rowMax.y + off),
                          ring, 11.0f, 0, 1.0f);
        }

        draw->AddRect(p, rowMax, ColorSuccess(), 10.0f, 0, 2.0f);
    }

    if (selected && active && !glowing)
        draw->AddRectFilled(p, ImVec2(p.x + 3, rowMax.y), ColorAccent(), 2.0f);

    draw->AddRect(p, rowMax,
                  glowing ? ColorSuccess() : (selected && active ? ColorAccent() : IM_COL32(48, 52, 64, 255)),
                  10.0f, 0, glowing ? 2.0f : 1.0f);

    // Icon
    const float iconSize = 34.0f;
    const ImVec2 iconMin(p.x + 14, p.y + (rowHeight - iconSize) * 0.5f);
    const ImVec2 iconMax(iconMin.x + iconSize, iconMin.y + iconSize);
    const ImU32 iconColor = glowing ? ColorSuccess()
                          : active ? (selected ? ColorAccent() : IM_COL32(180, 190, 210, 255))
                                   : IM_COL32(110, 116, 130, 255);
    DrawDeviceIcon(draw, icon, iconMin, iconMax, iconColor);

    // Text
    const float textX = iconMax.x + 14;
    const ImU32 nameColor = glowing ? IM_COL32(134, 239, 172, 255)
                          : active ? IM_COL32(238, 240, 245, 255)
                                   : IM_COL32(150, 155, 168, 255);
    draw->AddText(g_FontRegular, g_FontRegular->FontSize,
                  ImVec2(textX, p.y + 12), nameColor, name.c_str());

    if (!type.empty())
        draw->AddText(g_FontSmall, g_FontSmall->FontSize,
                      ImVec2(textX, p.y + 32), IM_COL32(148, 163, 184, 255), type.c_str());

    // Right side: "IN USE" glow badge when active, otherwise ON/OFF
    if (glowing)
    {
        const char* badge = "IN USE";
        const ImU32 badgeColor = ColorSuccess();
        const ImVec2 badgeText = ImGui::CalcTextSize(badge);
        const float badgeW = badgeText.x + 18;
        const float badgeH = 20.0f;
        const ImVec2 badgeMin(rowMax.x - badgeW - 12, p.y + (rowHeight - badgeH) * 0.5f);
        const ImVec2 badgeMax(badgeMin.x + badgeW, badgeMin.y + badgeH);

        draw->AddRectFilled(badgeMin, badgeMax, IM_COL32(34, 197, 94, 90), 10.0f);
        draw->AddRect(badgeMin, badgeMax, badgeColor, 10.0f, 0, 1.0f);
        draw->AddText(ImVec2(badgeMin.x + 9, badgeMin.y + 2), badgeColor, badge);
    }
    else
    {
        const char* badge = active ? "ON" : "OFF";
        const ImU32 badgeColor = active ? ColorSuccess() : IM_COL32(120, 126, 140, 255);
        const ImVec2 badgeText = ImGui::CalcTextSize(badge);
        const float badgeW = badgeText.x + 18;
        const float badgeH = 20.0f;
        const ImVec2 badgeMin(rowMax.x - badgeW - 12, p.y + (rowHeight - badgeH) * 0.5f);
        const ImVec2 badgeMax(badgeMin.x + badgeW, badgeMin.y + badgeH);

        draw->AddRectFilled(badgeMin, badgeMax, (badgeColor & 0x00FFFFFF) | 0x33000000, 10.0f);
        draw->AddRect(badgeMin, badgeMax, badgeColor, 10.0f, 0, 1.0f);
        draw->AddText(ImVec2(badgeMin.x + 9, badgeMin.y + 2), badgeColor, badge);
    }

    ImGui::PopID();
    return clicked;
}

}
