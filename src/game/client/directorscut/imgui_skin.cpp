#include "cbase.h"
#include "imgui_skin.h"
#include "memdbgon.h"

#define RGBA_MAX 255.f
#define VEC_ALPHA 1.f

char* ImGui_Skin::GetName()
{
	return "Dear ImGui Dark";
}

void ImGui_Skin::SetSkin(ImGuiStyle* style)
{
	// Default Dark
	ImGui::StyleColorsDark(style);
}

char* ImGui_Skin_Light::GetName()
{
	return "Dear ImGui Light";
}

void ImGui_Skin_Light::SetSkin(ImGuiStyle* style)
{
	// Default Light
	ImGui::StyleColorsLight(style);
}

char* ImGui_Skin_DX::GetName()
{
	return "Reminiscent";
}

void ImGui_Skin_DX::SetSkin(ImGuiStyle* style)
{
	// Valve Qt-like style
    style->FramePadding = ImVec2(13 , 7);
    style->WindowRounding = 4.f;

	ImVec4* colors = style->Colors;
	colors[ImGuiCol_Tab] = ImVec4((30.f/RGBA_MAX)*0.17f, (30.f/RGBA_MAX)*0.17f, (30.f / RGBA_MAX) * 0.17f, VEC_ALPHA);
	colors[ImGuiCol_TabActive] = ImVec4((54.f/RGBA_MAX)*0.17f, (54.f/RGBA_MAX)*0.17f, (54.f/RGBA_MAX)*0.17f, VEC_ALPHA);
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_TabActive];
	colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
	colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_Tab];
	colors[ImGuiCol_TitleBg] = ImVec4((41.f / RGBA_MAX)*0.14f, (41.f / RGBA_MAX)*0.14f, (41.f / RGBA_MAX)*0.14f, VEC_ALPHA);
	colors[ImGuiCol_TitleBgActive] = colors[ImGuiCol_TitleBg];
	colors[ImGuiCol_TitleBgCollapsed] = colors[ImGuiCol_TitleBg];
    colors[ImGuiCol_FrameBg] = colors[ImGuiCol_TitleBg];
    colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_TitleBg];
    colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_TitleBg];
    colors[ImGuiCol_Text] = ImVec4((182.f / RGBA_MAX) * 0.65f, (182.f / RGBA_MAX) * 0.65f, (182.f / RGBA_MAX) * 0.65f, VEC_ALPHA);
    colors[ImGuiCol_TextDisabled] = ImVec4((120.f / RGBA_MAX) * 0.65f, (120.f / RGBA_MAX) * 0.65f, (120.f / RGBA_MAX) * 0.65f, VEC_ALPHA);
    colors[ImGuiCol_Border] = colors[ImGuiCol_TitleBg];
    colors[ImGuiCol_WindowBg] = colors[ImGuiCol_TabActive];
    colors[ImGuiCol_Button] = ImVec4((84.f / RGBA_MAX) * 0.25f, (84.f / RGBA_MAX) * 0.25f, (84.f / RGBA_MAX) * 0.25f, VEC_ALPHA);

    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
