#include "imgui_public.h"

#ifndef IMGUI_SKIN
#define IMGUI_SKIN

static class ImGui_Skin
{
public:
	static char* GetName();
	static void SetSkin(ImGuiStyle* style);
};

static class ImGui_Skin_Light : ImGui_Skin
{
public:
	static char* GetName();
	static void SetSkin(ImGuiStyle* style);
};

static class ImGui_Skin_DX : ImGui_Skin
{
public:
	static char* GetName();
	static void SetSkin(ImGuiStyle* style);
};

#endif
