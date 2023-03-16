//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "directorscut.h"
#include "imgui_public.h"
#include "direct3d_hook.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Panel.h"
#include "iinput.h"
#include "input.h"
#include "view.h"
#include "viewrender.h"
#include "engine/IClientLeafSystem.h"
#include "view_shared.h"
#include "viewrender.h"
#include "gamestringpool.h"
#include "datacache/imdlcache.h"
#include "networkstringtable_clientdll.h"
#include "debugoverlay_shared.h"
#include "view_scene.h"
#include "in_buttons.h"
#include "filesystem.h"
#include "dmxloader/dmxloader.h"
#include "dmxloader/dmxelement.h"
#include <vgui/IInput.h>
#include <imgui_skin.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CDirectorsCutSystem g_DirectorsCutSystem;

OPENFILENAME ofn;

CDirectorsCutSystem &DirectorsCutGameSystem()
{
	return g_DirectorsCutSystem;
}

CElementPointer* CreateElement(bool add, bool setIndex, DAG_ type);
CElementPointer* CreateElement(bool add, bool setIndex, DAG_ type, char* name, KeyValues* kv);
void UnserializeKeyValuesDX(KeyValues* kv, bool append = false);
KeyValues* SerializeKeyValuesDX();
void SaveToKeyValues(char* path);
void LoadFromKeyValues(char* path, bool append = false);
void LoadTutorialFromFile(const char* filename);

ConVar cvar_imgui_menu_bar_visible("dx_imgui_menu_bar_visible", "1", FCVAR_ARCHIVE, "Enable ImGui menu bar");
ConVar cvar_imgui_visible("dx_imgui_visible", "1", FCVAR_ARCHIVE, "Enable ImGui visibility");
ConVar cvar_imgui_enabled("dx_imgui_enabled", "1", FCVAR_ARCHIVE, "Enable ImGui");
ConVar cvar_dx_zoom_distance("dx_zoom_distance", "5.0f", FCVAR_ARCHIVE, "Default zoom distance");
ConVar cvar_dx_orbit_sensitivity("dx_orbit_sensitivity", "0.1f", FCVAR_ARCHIVE, "Middle click orbit sensitivity");

ConCommand cmd_imgui_toggle("dx_imgui_toggle", [](const CCommand& args) {
	cvar_imgui_enabled.SetValue(!cvar_imgui_enabled.GetBool());
}, "Toggle ImGui", FCVAR_ARCHIVE);

ConCommand cmd_imgui_show("dx_imgui_show", [](const CCommand& args) {
	cvar_imgui_visible.SetValue(true);
	cvar_imgui_enabled.SetValue(true);
	// if arg 1 is 2, set main menu bool
	if (args.ArgC() > 1)
	{
		switch (args.Arg(1)[0])
		{
		case '2':
			g_DirectorsCutSystem.mainMenu = true;
			break;
#if DX_UNIVERSE > DX_UNIVERSE_PUBLIC
		// debug params
		case '3':
			g_DirectorsCutSystem.levelInit = true;
			break;
		case '4':
			g_DirectorsCutSystem.mainMenu = false;
			g_DirectorsCutSystem.levelInit = false;
			break;
#endif
		}
	}
}, "Show ImGui", FCVAR_ARCHIVE);

ConCommand cmd_imgui_hide("dx_imgui_hide", [](const CCommand& args) {
	cvar_imgui_visible.SetValue(false);
	cvar_imgui_enabled.SetValue(false);
}, "Hide ImGui", FCVAR_ARCHIVE);

WNDPROC ogWndProc = nullptr;

static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE | ImGuizmo::ROTATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

// Render Dear ImGui
void APIENTRY EndScene(LPDIRECT3DDEVICE9 p_pDevice)
{
	// Initialization
	//bool firstFrame = false;
	if (g_DirectorsCutSystem.firstEndScene)
	{
		g_DirectorsCutSystem.firstEndScene = false;
		//firstFrame = true;
		ImGui_ImplDX9_Init(p_pDevice);
		g_DirectorsCutSystem.fontTahoma = ImGui::GetIO().Fonts->AddFontFromFileTTF("tahoma.ttf", 13.0f);
		Assert(g_DirectorsCutSystem.fontTahoma);
	}
	// Only render Imgui when enabled
	if (cvar_imgui_enabled.GetBool() && g_DirectorsCutSystem.imguiActive && !engine->IsPaused())
	{
		g_DirectorsCutSystem.gotInput = false;

		bool justRemoved = false; // failsafe for removing elements
		bool mouseWheelTaken = false; // something captured the mouse wheel
		bool viewDirty = false; // whether or not to re-render the viewport

		ImGuiIO& io = ImGui::GetIO();

		int windowWidth;
		int windowHeight;
		vgui::surface()->GetScreenSize(windowWidth, windowHeight);

		// Input

		if(g_DirectorsCutSystem.levelInit)
		{
			// Pressing tab will toggle cvar_imgui_enabled
			if (ImGui::IsKeyPressed(ImGuiKey_Tab, false))
			{
				cvar_imgui_enabled.SetValue(false);
			}

			// Holding ctrl will set g_DirectorsCutSystem.selecting to true
			g_DirectorsCutSystem.selecting = (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl));
		}

		// Set Docking Mode
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Drawing
		
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();

		// Pressing F7 will toggle cvar_imgui_visible
		if (ImGui::IsKeyPressed(ImGuiKey_F7, false))
		{
			cvar_imgui_visible.SetValue(!cvar_imgui_visible.GetBool());
			// get r_drawvgui and set its value
			ConVar* r_drawvgui = cvar->FindVar("r_drawvgui");
			r_drawvgui->SetValue(cvar_imgui_visible.GetBool());
		}

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		// Set font
		ImGui::PushFont(g_DirectorsCutSystem.fontTahoma);

		if(cvar_imgui_visible.GetBool())
		{

			// ---- menu bar ----- //

			// Pressing F6 will toggle cvar_imgui_menu_bar_visible
			if (ImGui::IsKeyPressed(ImGuiKey_F6, false))
			{
				cvar_imgui_menu_bar_visible.SetValue(!cvar_imgui_menu_bar_visible.GetBool());
			}

			if (cvar_imgui_menu_bar_visible.GetBool() && ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File", g_DirectorsCutSystem.levelInit))
				{
					ImGui::Text("USE SAVING/LOADING AT YOUR OWN RISK!");
					ImGui::Text("Saving/loading is not fully implemented yet.");
					ImGui::Text("You may lose your progress.");
					ImGui::Text("You have been warned.");
					ImGui::Separator();
					bool dummy = false;
					if (ImGui::MenuItem("New", NULL))
					{
						KeyValues* kv = new KeyValues("DirectorsCut");
						KeyValues* version = kv->FindKey("version", true);
						version->SetInt("major", g_DirectorsCutSystem.directorcut_version.m_major);
						version->SetInt("minor", g_DirectorsCutSystem.directorcut_version.m_minor);
						version->SetInt("patch", g_DirectorsCutSystem.directorcut_version.m_patch);
						UnserializeKeyValuesDX(kv);
						kv->deleteThis();
						g_DirectorsCutSystem.SetDefaultSettings();
					}
					if (ImGui::MenuItem("Open..."))
					{
						// TODO: make this not so cluttered
						char szFile[MAX_PATH] = { 0 };
						ZeroMemory(&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = NULL;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile);
						ofn.lpstrFilter = "KeyValues (*.vdf)\0*.vdf\0KeyValues (*.kv2)\0*.kv2\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFileTitle = NULL;
						ofn.nMaxFileTitle = 0;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
						// pull from exe path (wapi)
						char exePath[MAX_PATH];
						GetModuleFileName(NULL, exePath, MAX_PATH);
						char* exePathEnd = strrchr(exePath, '\\');
						*exePathEnd = '\0';
						ofn.lpstrInitialDir = exePath;
						if (GetOpenFileName(&ofn))
						{
							sprintf(g_DirectorsCutSystem.savePath, "%s", ofn.lpstrFile);
							LoadFromKeyValues( ofn.lpstrFile );
							g_DirectorsCutSystem.savedOnce = true;
						}
					}
					if (ImGui::MenuItem("Append..."))
					{
						// TODO: make this not so cluttered
						char szFile[MAX_PATH] = { 0 };
						ZeroMemory(&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = NULL;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile);
						ofn.lpstrFilter = "KeyValues (*.vdf)\0*.vdf\0KeyValues (*.kv2)\0*.kv2\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFileTitle = NULL;
						ofn.nMaxFileTitle = 0;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
						// pull from exe path (wapi)
						char exePath[MAX_PATH];
						GetModuleFileName(NULL, exePath, MAX_PATH);
						char* exePathEnd = strrchr(exePath, '\\');
						*exePathEnd = '\0';
						ofn.lpstrInitialDir = exePath;
						if (GetOpenFileName(&ofn))
						{
							sprintf(g_DirectorsCutSystem.savePath, "%s", ofn.lpstrFile);
							LoadFromKeyValues(ofn.lpstrFile, true);
							g_DirectorsCutSystem.savedOnce = true;
						}
					}
					if (ImGui::MenuItem("Save", NULL, &dummy, g_DirectorsCutSystem.savedOnce))
					{
						SaveToKeyValues(g_DirectorsCutSystem.savePath);
					}
					if (ImGui::MenuItem("Save As..."))
					{
						// another long windows api call
						char szFile[MAX_PATH] = { 0 };
						ZeroMemory(&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = NULL;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile);
						ofn.lpstrFilter = "KeyValues (*.vdf)\0*.kv2\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFileTitle = NULL;
						ofn.nMaxFileTitle = 0;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
						// pull from exe path (wapi)
						char exePath[MAX_PATH];
						GetModuleFileName(NULL, exePath, MAX_PATH);
						char* exePathEnd = strrchr(exePath, '\\');
						*exePathEnd = '\0';
						ofn.lpstrInitialDir = exePath;
						if (GetSaveFileName(&ofn))
						{
							sprintf(g_DirectorsCutSystem.savePath, "%s", ofn.lpstrFile);
							// Check if file extension is present (either .vdf or .kv2)
							char* fileExtension = strrchr(ofn.lpstrFile, '.');
							if (fileExtension == NULL)
							{
								// If not, append .vdf
								strcat(ofn.lpstrFile, ".vdf");
							}
							else
							{
								// If so, check if it's .vdf or .kv2
								if (strcmp(fileExtension, ".vdf") != 0 && strcmp(fileExtension, ".kv2") != 0)
								{
									// We're not going to append it because of uncertainty
									// Just gonna send a warning to the console
									Msg("DX Warning: File extension is not .vdf or .kv2. Saving anyway...\n");
									//strcat(ofn.lpstrFile, ".vdf");
								}
							}
							SaveToKeyValues(ofn.lpstrFile);
							g_DirectorsCutSystem.savedOnce = true;
						}
					}
					if (ImGui::MenuItem("Reload", NULL, &dummy, g_DirectorsCutSystem.savedOnce))
					{
						LoadFromKeyValues(g_DirectorsCutSystem.savePath);
					}
					if (ImGui::MenuItem("Reappend", NULL, &dummy, g_DirectorsCutSystem.savedOnce))
					{
						LoadFromKeyValues(g_DirectorsCutSystem.savePath, true);
					}
					if (ImGui::MenuItem("Exit"))
					{
						// Call clientcmd to disconnect from server
						engine->ClientCmd_Unrestricted("disconnect");
						g_DirectorsCutSystem.levelInit = false;
						g_DirectorsCutSystem.mainMenu = true;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit", g_DirectorsCutSystem.levelInit))
				{
					bool dummy = false;
					if (ImGui::MenuItem("Undo", NULL, &dummy, false))
					{
					}
					if (ImGui::MenuItem("Redo", NULL, &dummy, false))
					{
					}
					ImGui::SliderFloat("Time Scale", &g_DirectorsCutSystem.timeScale, 0.01f, 1.f, "%.2f");
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("View"))
				{
					// windows -> show/hide camera window, show/hide element window, show/hide inspector window
					if(g_DirectorsCutSystem.levelInit)
					{
						ImGui::MenuItem("Editor Window", NULL, &g_DirectorsCutSystem.windowVisibilities[0]);
						ImGui::MenuItem("Elements Window", NULL, &g_DirectorsCutSystem.windowVisibilities[1]);
						ImGui::MenuItem("Inspector Window", NULL, &g_DirectorsCutSystem.windowVisibilities[2]);
					}
					ImGui::MenuItem("Tutorial Window", NULL, &g_DirectorsCutSystem.windowVisibilities[3]);
					ImGui::EndMenu();
				}
				if(ImGui::BeginMenu("About"))
				{
					// external link buttons
					if (ImGui::MenuItem("Discord"))
						ShellExecute(0, 0, "https://discord.gg/3X3teNecWs", 0, 0, SW_SHOW);
#if DX_UNIVERSE > 2
					if (ImGui::MenuItem("GitHub"))
						ShellExecute(0, 0, "https://github.com/SFMDX/DirectorsCut-Swarm", 0, 0, SW_SHOW);
#endif
					if (ImGui::MenuItem("Twitter"))
						ShellExecute(0, 0, "https://twitter.com/SFMDirectorsCut", 0, 0 , SW_SHOW );
					// Origin of build
					ImGui::Text("%s: %s", DX_UNIVERSE_TEXT, DX_UNIVERSE_ORIGIN);
					// Version of build
					ImGui::Text("Version: v%s", g_DirectorsCutSystem.directorcut_version.GetVersion());
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			// ----- windows ----- //

			if(g_DirectorsCutSystem.levelInit)
			{
				// element viewer (tree window)
				// set up window
				if (g_DirectorsCutSystem.windowVisibilities[1])
				{
					/*
					bool docked = g_DirectorsCutSystem.inspectorDocked;
					ImGui::SetNextWindowPos(ImVec2(windowWidth - 8, 19 + 8), 0, ImVec2(1.0f, 0.0f));
					if (docked)
					{
						ImGui::SetNextWindowSize(ImVec2(264, windowHeight - 96 - 19 - 24), 0);
					}
					else
					{
						ImGui::SetNextWindowSizeConstraints(ImVec2(264, windowHeight - 96 - 19 - 24), ImVec2(windowWidth, windowHeight - 96 - 19 - 24));
					}
					*/

					ImGui::Begin("Elements"); //(!docked ? ImGuiWindowFlags_AlwaysAutoResize : ImGuiWindowFlags_NoResize) | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar);

					if (ImGui::IsWindowHovered())
					{
						// check for mouse wheel input
						if (io.MouseWheel != 0)
							mouseWheelTaken = true;
					}

					if (ImGui::BeginMenu("Create Element"))
					{
						if (ImGui::BeginMenu("Model"))
						{
							if (ImGui::MenuItem(g_DirectorsCutSystem.modelName))
								CreateElement(true, true, DAG_MODEL);

							//if (ImGui::MenuItem("Enter Model Name"))
								//ImGui::OpenPopup("ModelEntry");
							//if (ImGui::MenuItem("Open Model Browser"))
								//ImGui::OpenPopup("ModelBrowser");

							if (ImGui::MenuItem("Open File Browser"))
							{
								// it would be too tasking to add imgui file browser, so we'll just use the windows one
								OPENFILENAME ofn;
								char szFile[MAX_PATH] = { 0 };
								ZeroMemory(&ofn, sizeof(ofn));
								ofn.lStructSize = sizeof(ofn);
								ofn.hwndOwner = NULL;
								ofn.lpstrFile = szFile;
								ofn.nMaxFile = sizeof(szFile);
								// filter for .mdl files
								ofn.lpstrFilter = "Model Files (*.mdl)\0*.mdl\0";
								ofn.nFilterIndex = 1;
								ofn.lpstrFileTitle = NULL;
								ofn.nMaxFileTitle = 0;
								ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
								// pull from exe path (wapi)
								char exePath[MAX_PATH];
								GetModuleFileName(NULL, exePath, MAX_PATH);
								char* exePathEnd = strrchr(exePath, '\\');
								*exePathEnd = '\0';
								ofn.lpstrInitialDir = exePath;
								if (GetOpenFileName(&ofn))
								{
									// relative path to the model
									char* relativePath = strstr(szFile, "models\\");
									// replace backslashes with forward slashes
									for (int i = 0; i < (int)strlen(relativePath); i++)
									{
										if (relativePath[i] == '\\')
											relativePath[i] = '/';
									}
									// create the element
									sprintf(g_DirectorsCutSystem.modelName, "%s", relativePath);
									CreateElement(true, true, DAG_MODEL);
								}
							}
							ImGui::EndMenu();
						}
						if (ImGui::MenuItem("Light"))
							CreateElement(true, true, DAG_LIGHT);
						if (ImGui::MenuItem("Camera"))
							CreateElement(true, true, DAG_CAMERA);
						if (ImGui::MenuItem("Generic"))
							CreateElement(true, true, DAG_NONE);
						ImGui::EndMenu();
					}

					// these two don't work, for now

					if (ImGui::BeginPopup("ModelEntry", ImGuiWindowFlags_AlwaysAutoResize))
					{
						if (ImGui::InputText("Model Name", g_DirectorsCutSystem.modelName, CHAR_MAX))
							g_DirectorsCutSystem.gotInput = true;
						if (ImGui::Button("OK"))
							CreateElement(true, true, DAG_MODEL);
						ImGui::EndPopup();
					}

					if (ImGui::BeginPopup("ModelBrowser", ImGuiWindowFlags_AlwaysAutoResize))
					{
						ImGui::Text("Placeholder for model browser!");
						ImGui::Text("Use model entry or file browser to load models.");
						ImGui::EndPopup();
					}

					/*
					if (docked)
					{
						if (ImGui::Button("Undock"))
							g_DirectorsCutSystem.inspectorDocked = false;
					}
					else
					{
						if (ImGui::Button("Dock"))
							g_DirectorsCutSystem.inspectorDocked = true;
					}

					ImGui::SameLine();
					ImGui::Checkbox("Spawn at Pivot", &g_DirectorsCutSystem.spawnAtPivot);
					*/

					// child for element tree
					ImGui::BeginChild("ElementTree", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

					int flags = ImGuiTreeNodeFlags_DefaultOpen;

					if (ImGui::TreeNodeEx("Pivot", flags | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf | (g_DirectorsCutSystem.elementIndex < 0 ? ImGuiTreeNodeFlags_Selected : 0)))
					{
						if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1))
							g_DirectorsCutSystem.elementIndex = -1;
						if (g_DirectorsCutSystem.elementIndex < 0)
						{
							if (ImGui::BeginPopupContextWindow())
							{
								ImGui::Text("***** PIVOT *****");
								// reset pivot
								if (ImGui::MenuItem("Reset Pivot"))
									g_DirectorsCutSystem.pivot = Vector(0, 0, 0);
								ImGui::Text("Origin: %.2f %.2f %.2f", g_DirectorsCutSystem.pivot.x, g_DirectorsCutSystem.pivot.y, g_DirectorsCutSystem.pivot.z);
								ImGui::EndPopup();
							}
						}
						ImGui::TreePop();
					}

					if (g_DirectorsCutSystem.elements.Count() > 0)
					{
						for (int i = 0; i < g_DirectorsCutSystem.elements.Count(); i++)
						{
							CElementPointer* element = g_DirectorsCutSystem.elements[i];
							if (element == nullptr)
								break;
							DAG_ dagType = element->GetType();
							void* elementPtr = element->GetPointer();
							if (elementPtr == nullptr)
								break;
							C_BaseEntity* dag = (C_BaseEntity*)elementPtr;
							if (dag == nullptr)
								break;
							// use i as name for tree node
							char name[CHAR_MAX];
							sprintf(name, "%d", i);
							// if element->name is not empty, use that instead
							if (element->name[0] != '\0')
								sprintf(name, "%s (%s)", name, element->name);
							switch (element->GetType())
							{
							case DAG_MODEL:
							{
								CModelElement* model = (CModelElement*)element->GetPointer();
								if (model != nullptr)
									sprintf(name, "%s (%s)", name, STRING(model->GetModelPtr()->pszName()));
								break;
							}
							case DAG_LIGHT:
								sprintf(name, "%s (Light)", name);
								break;
							case DAG_CAMERA:
								sprintf(name, "%s (Camera)", name);
								break;
							default:
								sprintf(name, "%s", name);
								break;
							}
							if (ImGui::TreeNodeEx(name, flags | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf | (dagType == DAG_MODEL ? 0 : ImGuiTreeNodeFlags_Leaf) | ((i == g_DirectorsCutSystem.elementIndex) ? ImGuiTreeNodeFlags_Selected : 0)))
							{
								if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1))
								{
									g_DirectorsCutSystem.elementIndex = i;
									g_DirectorsCutSystem.boneIndex = -1;
									g_DirectorsCutSystem.nextPoseIndex = -1;
									g_DirectorsCutSystem.flexIndex = -1;
								}
								if (g_DirectorsCutSystem.elementIndex == i)
								{
									if (ImGui::BeginPopupContextItem("Element"))
									{
										switch (element->GetType())
										{
										case DAG_MODEL:
										{
											ImGui::Text("***** MODEL *****");
											break;
										}
										case DAG_LIGHT:
											ImGui::Text("***** LIGHT *****");
											break;
										case DAG_CAMERA:
											ImGui::Text("***** CAMERA *****");
											break;
										default:
											ImGui::Text("***** ELEMENT *****");
											break;
										}
										Vector origin = dag->GetAbsOrigin();
										QAngle angles = dag->GetAbsAngles();
										// remove element
										if (ImGui::MenuItem("Remove"))
										{
											delete element;
											g_DirectorsCutSystem.elements.Remove(g_DirectorsCutSystem.elementIndex);
											g_DirectorsCutSystem.elementIndex = g_DirectorsCutSystem.elements.Count() - 1;
											g_DirectorsCutSystem.boneIndex = -1;
											g_DirectorsCutSystem.nextPoseIndex = -1;
											g_DirectorsCutSystem.flexIndex = -1;
											justRemoved = true;
										}
										ImGui::Text("Origin: %.2f %.2f %.2f", origin.x, origin.y, origin.z);
										ImGui::Text("Angles: %f %f %f", angles.x, angles.y, angles.z);
										ImGui::EndPopup();
									}
								}
								if (!justRemoved)
								{
									if (dagType == DAG_MODEL)
									{
										CModelElement* modelDag = (CModelElement*)elementPtr;
										CStudioHdr* modelPtr = modelDag->GetModelPtr();
										if (modelPtr)
										{
											// show pose bones
											/*
											if (ImGui::TreeNodeEx("Pose Bones", flags))
											{
												*/
												for (int j = 0; j < modelPtr->numbones(); j++)
												{
													mstudiobone_t* bone = modelPtr->pBone(j);
													if (bone != nullptr)
													{
														char name[CHAR_MAX];
														sprintf(name, "%d (%s)", j, bone->pszName());
														if (ImGui::TreeNodeEx(name, flags | ImGuiTreeNodeFlags_Leaf | ((i == g_DirectorsCutSystem.elementIndex && j == g_DirectorsCutSystem.nextPoseIndex) ? ImGuiTreeNodeFlags_Selected : 0)))
														{
															if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1))
															{
																g_DirectorsCutSystem.elementIndex = i;
																g_DirectorsCutSystem.boneIndex = -1;
																g_DirectorsCutSystem.nextPoseIndex = j;
																g_DirectorsCutSystem.flexIndex = -1;
															}
															if (g_DirectorsCutSystem.elementIndex == i && g_DirectorsCutSystem.nextPoseIndex == j)
															{
																if (ImGui::BeginPopupContextItem("Pose Bone"))
																{
																	ImGui::Text("***** POSE BONE *****");
																	ImGui::Text("Origin: %.2f %.2f %.2f", g_DirectorsCutSystem.poseBoneOrigin.x, g_DirectorsCutSystem.poseBoneOrigin.y, g_DirectorsCutSystem.poseBoneOrigin.z);
																	ImGui::Text("Angles: %f %f %f", g_DirectorsCutSystem.poseBoneAngles.x, g_DirectorsCutSystem.poseBoneAngles.y, g_DirectorsCutSystem.poseBoneAngles.z);
																	ImGui::EndPopup();
																}
															}
															// FIXME: can't show position in drawing code!
															ImGui::TreePop();
														}
													}
												}
											/*
												ImGui::TreePop();
											}
											// show flex controllers
											if (ImGui::TreeNodeEx("Flex Controllers", flags))
											{
												for (int j = 0; j < modelPtr->numflexcontrollers(); j++)
												{
													mstudioflexcontroller_t* pflexcontroller = modelPtr->pFlexcontroller((LocalFlexController_t)j);
													char flexName[CHAR_MAX];
													sprintf(flexName, "%d (%s)", j, (char*)modelDag->GetFlexControllerName((LocalFlexController_t)j));
													if (ImGui::TreeNodeEx(flexName, flags | ImGuiTreeNodeFlags_Leaf | ((i == g_DirectorsCutSystem.elementIndex && j == g_DirectorsCutSystem.flexIndex) ? ImGuiTreeNodeFlags_Selected : 0)))
													{
														if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1))
														{
															g_DirectorsCutSystem.elementIndex = i;
															g_DirectorsCutSystem.boneIndex = -1;
															g_DirectorsCutSystem.nextPoseIndex = -1;
															g_DirectorsCutSystem.flexIndex = j;
														}
														if (g_DirectorsCutSystem.elementIndex == i && g_DirectorsCutSystem.flexIndex == j)
														{
															if (ImGui::BeginPopupContextItem("Flex Controller"))
															{
																ImGui::Text("***** FLEX CONTROLLER *****");
																ImGui::Text("Weight: %.2f", modelDag->GetFlexWeight((LocalFlexController_t)j));
																ImGui::Text("Range: %.2f min %.2f max", pflexcontroller->min, pflexcontroller->max);
																ImGui::EndPopup();
															}
														}
														ImGui::TreePop();
													}
												}
												ImGui::TreePop();
											}
											*/
										}
									}
								}
								ImGui::TreePop();
							}
						}
					}

					// end child
					ImGui::EndChild();

					// end window
					ImGui::End();
				}

				// Inspector window
				if (g_DirectorsCutSystem.windowVisibilities[2])
				{
					ImGui::Begin("Inspector", 0, ImGuiWindowFlags_AlwaysAutoResize);

					if (g_DirectorsCutSystem.elementIndex >= 0 && g_DirectorsCutSystem.elementIndex < g_DirectorsCutSystem.elements.Count())
					{
						CElementPointer* element = g_DirectorsCutSystem.elements[g_DirectorsCutSystem.elementIndex];
						if (element != nullptr)
						{
							DAG_ dagType = element->GetType();
							void* elementPtr = element->GetPointer();
							if (elementPtr != nullptr)
							{
								C_BaseEntity* dag = (C_BaseEntity*)elementPtr;
								if (dag != nullptr)
								{
									ImGui::Text("Generic:");
									ImGui::Separator();

									// Generic attributes:
									// Name
									// Position
									// Rotation
									
									// Text box for name
									char* name = element->name;
									if(ImGui::InputText("Name", name, CHAR_MAX))
										g_DirectorsCutSystem.gotInput = true;
									if (strcmp(name, element->name) != 0)
										strcpy(element->name, name);

									// Position
									Vector positionV = dag->GetAbsOrigin();
									float position[3];
									position[0] = positionV.x;
									position[1] = positionV.y;
									position[2] = positionV.z;
									if (ImGui::InputFloat3("Position", position))
									{
										dag->SetAbsOrigin(Vector(position[0], position[1], position[2]));
									}

									// Rotation
									QAngle rotationA = dag->GetAbsAngles();
									float rotation[3];
									rotation[0] = rotationA.x;
									rotation[1] = rotationA.y;
									rotation[2] = rotationA.z;
									if (ImGui::InputFloat3("Rotation", rotation))
									{
										dag->SetAbsAngles(QAngle(rotation[0], rotation[1], rotation[2]));
									}

									// Type-specific
									switch(dagType)
									{
										case DAG_MODEL:
											{
												CModelElement* modelDag = (CModelElement*)elementPtr;

												if (modelDag == nullptr)
												{
													ImGui::Text("Model not found!");
													break;
												}

												CStudioHdr* modelPtr = modelDag->GetModelPtr();
												if (modelPtr)
												{
													// Model attributes:
													// Skin

													ImGui::Text("Model: ", STRING(modelPtr->pszName()));
													ImGui::Separator();

													// Skin
													int skin = modelDag->GetSkin();
													if (ImGui::InputInt("Skin", &skin))
													{
														modelDag->SetSkin(skin);
													}

													// Bones
													for (int i = 0; i < modelPtr->numbones(); i++)
													{
														mstudiobone_t* bone = modelPtr->pBone(i);
														if (bone != nullptr)
														{
															if (i == g_DirectorsCutSystem.nextPoseIndex)
															{
																ImGui::Text("Bone: %s", bone->pszName());
																ImGui::Separator();

																float position[3];
																position[0] = g_DirectorsCutSystem.poseBoneOrigin.x;
																position[1] = g_DirectorsCutSystem.poseBoneOrigin.y;
																position[2] = g_DirectorsCutSystem.poseBoneOrigin.z;
																if(ImGui::InputFloat3("Position", position))
																{
																	g_DirectorsCutSystem.poseBoneOrigin.x = position[0];
																	g_DirectorsCutSystem.poseBoneOrigin.y = position[1];
																	g_DirectorsCutSystem.poseBoneOrigin.z = position[2];
																}

																float rotation[3];
																rotation[0] = g_DirectorsCutSystem.poseBoneAngles.x;
																rotation[1] = g_DirectorsCutSystem.poseBoneAngles.y;
																rotation[2] = g_DirectorsCutSystem.poseBoneAngles.z;
																if(ImGui::InputFloat3("Rotation", rotation))
																{
																	g_DirectorsCutSystem.poseBoneAngles.x = rotation[0];
																	g_DirectorsCutSystem.poseBoneAngles.y = rotation[1];
																	g_DirectorsCutSystem.poseBoneAngles.z = rotation[2];
																}

																break;
															}
														}
													}
													if (g_DirectorsCutSystem.flexIndex >= 0)
													{
														for (int i = 0; i < (int)modelPtr->numflexcontrollers(); i++)
														{
															if (i == g_DirectorsCutSystem.flexIndex)
															{
																mstudioflexcontroller_t* pflexcontroller = modelPtr->pFlexcontroller((LocalFlexController_t)i);
																char* flexName = (char*)modelDag->GetFlexControllerName((LocalFlexController_t)i);

																ImGui::Text("Flex: ", flexName);
																ImGui::Separator();

																float flex = modelDag->GetFlexWeight((LocalFlexController_t)i);
																if (pflexcontroller->max != pflexcontroller->min)
																{
																	ImGui::SliderFloat("Weight", &flex, pflexcontroller->min, pflexcontroller->max);
																	modelDag->SetFlexWeight((LocalFlexController_t)i, flex);
																}
																else
																{
																	ImGui::LabelText("Weight", "%d", flex);
																}
																break;
															}
														}
													}
												}
												else
												{
													ImGui::Text("Model not found!");
												}
											}
											break;
										case DAG_LIGHT:
											{
												CLightElement* lightDag = (CLightElement*)elementPtr;

												if (lightDag == nullptr)
												{
													ImGui::Text("Light not found!");
													break;
												}

												ImGui::Text("Light");
												ImGui::Separator();

												// Light attributes:

											}
											break;
										case DAG_CAMERA:
											{
												CCameraElement* cameraDag = (CCameraElement*)elementPtr;

												if (cameraDag == nullptr)
												{
													ImGui::Text("Camera not found!");
													break;
												}

												ImGui::Text("Camera");
												ImGui::Separator();

												// Camera attributes:

											}
											break;
									}
								}
							}
						}
					}
					else if (g_DirectorsCutSystem.elementIndex == -1)
					{
						ImGui::Text("Pivot");
						ImGui::Separator();

						// Pivot attributes:
						// Position

						// Position
						Vector positionV = g_DirectorsCutSystem.pivot;
						float position[3];
						position[0] = positionV.x;
						position[1] = positionV.y;
						position[2] = positionV.z;
						if (ImGui::InputFloat3("Position", position))
						{
							g_DirectorsCutSystem.pivot = Vector(position[0], position[1], position[2]);
						}
					}
					else 
					{
						ImGui::Text("No element selected");
					}

					// end window
					ImGui::End();
				}

				if (!justRemoved)
				{
					// continue, otherwise stop everything

					// Perspective camera view
					// Orthographic mode is not supported
					const CViewSetup* viewSetup = view->GetViewSetup();
					float fov = (g_DirectorsCutSystem.fov != g_DirectorsCutSystem.fovDefault) ? g_DirectorsCutSystem.fov : viewSetup->fov * g_DirectorsCutSystem.fovAdjustment;
					g_DirectorsCutSystem.Perspective(fov, io.DisplaySize.x / io.DisplaySize.y, 0.01f, viewSetup->zFar, g_DirectorsCutSystem.cameraProjection); //viewSetup->zNear, viewSetup->zFar, g_DirectorsCutSystem.cameraProjection);

					ImGuizmo::SetOrthographic(g_DirectorsCutSystem.orthographic);

					if (g_DirectorsCutSystem.windowVisibilities[0])
					{
						ImGui::Begin("Editor", 0, ImGuiWindowFlags_AlwaysAutoResize);

						bool taken = false;
						bool key_q = ImGui::IsKeyPressed(ImGuiKey_Q, false);

						ImGui::Text("Controls");
						ImGui::Separator();

						// Element pivot change
						if (g_DirectorsCutSystem.elementIndex > -1)
						{
							CElementPointer* element;
							if (g_DirectorsCutSystem.elementIndex > g_DirectorsCutSystem.elements.Count() - 1)
								g_DirectorsCutSystem.elementIndex = g_DirectorsCutSystem.elements.Count() - 1;
							if (g_DirectorsCutSystem.elements.Count() > 0)
								element = g_DirectorsCutSystem.elements[g_DirectorsCutSystem.elementIndex];
							else
								element = nullptr;
							if (element != nullptr)
							{
								DAG_ dagType = element->GetType();
								void* elementPtr = element->GetPointer();
								if (elementPtr != nullptr)
								{
									C_BaseEntity* dag = (C_BaseEntity*)elementPtr;
									if (dag != nullptr)
									{
										if (dagType == DAG_MODEL)
										{
											CModelElement* modelDag = (CModelElement*)elementPtr;
											if (modelDag != nullptr)
											{
												if (!taken && g_DirectorsCutSystem.nextPoseIndex > -1)
												{
													if (ImGui::Button("Selected Pose Bone to Pivot (Q)") || (!g_DirectorsCutSystem.gotInput && key_q))
													{
														g_DirectorsCutSystem.gotInput = true;
														g_DirectorsCutSystem.pivot = g_DirectorsCutSystem.poseBoneOrigin;
													}
													taken = true;
												}
											}
										}
										if (!taken)
										{
											if (ImGui::Button("Selected Element to Pivot (Q)") || (!g_DirectorsCutSystem.gotInput && key_q))
											{
												g_DirectorsCutSystem.gotInput = true;
												g_DirectorsCutSystem.pivot = dag->GetAbsOrigin();
											}
											taken = true;
										}
									}
								}
							}
						}

						if (!taken)
						{
							if (ImGui::Button("Map Origin to Pivot (Q)") || (!g_DirectorsCutSystem.gotInput && key_q))
							{
								g_DirectorsCutSystem.gotInput = true;
								g_DirectorsCutSystem.pivot = Vector(0, 0, 0);
							}
						}

						if (ImGui::SliderFloat("Distance", &g_DirectorsCutSystem.distance, 5.f, 5000.f, "%.0f"))
							viewDirty = true;

						// TODO: camera dags should unlock field of view slider and hide gizmos
						ImGui::SliderFloat("Field of View", &g_DirectorsCutSystem.fov, 1.f, 179.f, "%.0f");

						if(ImGui::InputFloat3("Snap", g_DirectorsCutSystem.snap))
							g_DirectorsCutSystem.gotInput = true;
						ImGui::SameLine();
						ImGui::Checkbox("", &g_DirectorsCutSystem.useSnap);

						ImGui::Text("Transform Mode");
						ImGui::Separator();

						// Set disabled if fov != fovDefault
						if (g_DirectorsCutSystem.fov != g_DirectorsCutSystem.fovDefault)
						{
							// Text
							ImGui::Text("Field of view is overriding gizmo mode");
							// Button (set default)
							if (ImGui::Button("Set Default"))
							{
								g_DirectorsCutSystem.fov = g_DirectorsCutSystem.fovDefault;
							}
							// Set g_DirectorsCutSystem.operation to -1
							g_DirectorsCutSystem.operation = -1;
						}

						// Operation (gizmo mode)
						if (ImGui::RadioButton("None", g_DirectorsCutSystem.operation < 0))
							g_DirectorsCutSystem.operation = -1;
						if (ImGui::RadioButton("Translate", g_DirectorsCutSystem.operation == 0 && g_DirectorsCutSystem.fov == g_DirectorsCutSystem.fovDefault))
							g_DirectorsCutSystem.operation = 0;
						if (ImGui::RadioButton("Rotate", g_DirectorsCutSystem.operation == 1 && g_DirectorsCutSystem.fov == g_DirectorsCutSystem.fovDefault))
							g_DirectorsCutSystem.operation = 1;
						if (ImGui::RadioButton("Universal", g_DirectorsCutSystem.operation == 2 && g_DirectorsCutSystem.fov == g_DirectorsCutSystem.fovDefault))
							g_DirectorsCutSystem.operation = 2;

						switch (g_DirectorsCutSystem.operation)
						{
						case 0:
							mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
							break;
						case 1:
							mCurrentGizmoOperation = ImGuizmo::ROTATE;
							break;
						case 2:
							mCurrentGizmoOperation = ImGuizmo::TRANSLATE | ImGuizmo::ROTATE;
							break;
						}

						ImGui::Text("Movement Space");
						ImGui::Separator();

						// Global space toggles
						if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
							mCurrentGizmoMode = ImGuizmo::WORLD;
						ImGui::SameLine();
						if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
							mCurrentGizmoMode = ImGuizmo::LOCAL;

						if (g_DirectorsCutSystem.elementIndex > -1)
						{
							CElementPointer* element = g_DirectorsCutSystem.elements[g_DirectorsCutSystem.elementIndex];
							if (element != nullptr)
							{
								ImGui::Text("Selection");
								ImGui::Separator();
								if (g_DirectorsCutSystem.elements.Count() > 0)
								{
									// failsafe
									if (g_DirectorsCutSystem.elementIndex < 0)
										g_DirectorsCutSystem.elementIndex = 0;

									ImGui::SliderInt("Element Index", &g_DirectorsCutSystem.elementIndex, 0, g_DirectorsCutSystem.elements.Count() - 1);

									switch (element->GetType())
									{
									case DAG_MODEL:
										CModelElement* pEntity = (CModelElement*)element->GetPointer();
										if (pEntity != nullptr)
										{
											CStudioHdr* modelPtr = pEntity->GetModelPtr();
											if (modelPtr)
											{
												for (int i = 0; i < modelPtr->numbones(); i++)
												{
													mstudiobone_t* bone = modelPtr->pBone(i);
													if (bone != nullptr)
													{
														if (i == g_DirectorsCutSystem.nextPoseIndex)
														{
															ImGui::SliderInt("Pose Bone Index", &g_DirectorsCutSystem.nextPoseIndex, 0, modelPtr->numbones() - 1);
															char* boneName = bone->pszName();
															ImGui::LabelText("Pose Bone Name", "%s", boneName);
															break;
														}
													}
												}
											}
										}
										break;
									}
								}
							}
						}
						ImGui::End();
					}

					ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
					ImGuizmo::SetRect(0, 0, windowWidth, windowHeight);

					// Obsolete due to middle mouse orbiting
					//ImGuizmo::ViewManipulate(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.distance, ImVec2(windowWidth - 192, 0), ImVec2(192, 192), 0x10101010);

					// local space toggle
					if (!g_DirectorsCutSystem.gotInput && ImGui::IsKeyPressed(ImGuiKey_L, false))
					{
						g_DirectorsCutSystem.gotInput = true;
						if (mCurrentGizmoMode == ImGuizmo::LOCAL)
							mCurrentGizmoMode = ImGuizmo::WORLD;
						else
							mCurrentGizmoMode = ImGuizmo::LOCAL;
					}

					// camera movement controls

					if (!mouseWheelTaken)
					{
						float oldDistance = g_DirectorsCutSystem.distance;
						float mouseDistance = -io.MouseWheel * cvar_dx_zoom_distance.GetFloat();
						float newDistance = g_DirectorsCutSystem.distance + mouseDistance;

						if (newDistance >= 5.f && newDistance <= 5000.f)
							g_DirectorsCutSystem.distance = newDistance;
						else if (newDistance < 5.f)
							g_DirectorsCutSystem.distance = 5.f;
						else if (newDistance > 5000.f)
							g_DirectorsCutSystem.distance = 5000.f;

						if (oldDistance != g_DirectorsCutSystem.distance)
							viewDirty = true;
					}

					// Camera movement
					if (ImGui::IsMouseDown(2))
					{
						float sensitivity;
						Vector2D mouseDelta = Vector2D(io.MouseDelta.x, io.MouseDelta.y);

						// Middle click orbit
						sensitivity = cvar_dx_orbit_sensitivity.GetFloat();
						g_DirectorsCutSystem.camYAngle += mouseDelta.x * sensitivity;
						g_DirectorsCutSystem.camXAngle += mouseDelta.y * sensitivity;

						viewDirty = true;
					}

					// Reset camera view to accommodate new distance
					//if (viewDirty || firstFrame)
					//{
						float eye[3];
						eye[0] = cosf(g_DirectorsCutSystem.camYAngle) * cosf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance;
						eye[1] = sinf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance;
						eye[2] = sinf(g_DirectorsCutSystem.camYAngle) * cosf(g_DirectorsCutSystem.camXAngle) * g_DirectorsCutSystem.distance;
						float at[] = { 0.f, 0.f, 0.f };
						float up[] = { 0.f, 1.f, 0.f };
						g_DirectorsCutSystem.LookAt(eye, at, up, g_DirectorsCutSystem.cameraView);
						//firstFrame = false;
					//}

					// Gizmos
					if (g_DirectorsCutSystem.operation >= 0)
					{
						if (g_DirectorsCutSystem.elementIndex < 0)
						{
							// TODO: don't repeat this code
							float modelMatrixPivot[16];
							Vector posPivot = g_DirectorsCutSystem.newPivot;
							QAngle angPivot = QAngle();
							float sPivot = 1.f;
							static float translationPivot[3], rotationPivot[3], scalePivot[3];

							// pivot mode
							ImGuizmo::SetID(-1);

							// Add offset (pivot) to translation
							posPivot -= g_DirectorsCutSystem.pivot;

							// Y up to Z up - pos is Z up, translation is Y up
							translationPivot[0] = -posPivot.y;
							translationPivot[1] = posPivot.z;
							translationPivot[2] = -posPivot.x;

							rotationPivot[0] = -angPivot.x;
							rotationPivot[1] = angPivot.y;
							rotationPivot[2] = -angPivot.z;

							scalePivot[0] = sPivot;
							scalePivot[1] = sPivot;
							scalePivot[2] = sPivot;

							ImGuizmo::RecomposeMatrixFromComponents(translationPivot, rotationPivot, scalePivot, modelMatrixPivot);
							float deltaMatrix[16];
							float deltaTranslation[3], deltaRotation[3], deltaScale[3];
							ImGuizmo::Manipulate(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, modelMatrixPivot, deltaMatrix, (g_DirectorsCutSystem.useSnap ? g_DirectorsCutSystem.snap : NULL), NULL, NULL);
							ImGuizmo::DecomposeMatrixToComponents(deltaMatrix, deltaTranslation, deltaRotation, deltaScale);
							// Revert Z up
							float translation2[3];
							translation2[0] = -deltaTranslation[2];
							translation2[1] = -deltaTranslation[0];
							translation2[2] = deltaTranslation[1];
							float rotation2[3];
							rotation2[0] = -deltaRotation[0];
							rotation2[1] = deltaRotation[1];
							rotation2[2] = -deltaRotation[2];
							if (ImGuizmo::IsUsing())
							{
								g_DirectorsCutSystem.newPivot += Vector(translation2[0], translation2[1], translation2[2]);
								g_DirectorsCutSystem.justSetPivot = true;
							}
							else if (g_DirectorsCutSystem.justSetPivot)
							{
								g_DirectorsCutSystem.pivot = g_DirectorsCutSystem.newPivot;
								g_DirectorsCutSystem.justSetPivot = false;
							}
							else
							{
								g_DirectorsCutSystem.newPivot = g_DirectorsCutSystem.pivot;
							}
						}
						else if (g_DirectorsCutSystem.elementIndex > -1)
						{
							CElementPointer* element = g_DirectorsCutSystem.elements[g_DirectorsCutSystem.elementIndex];
							if (element != nullptr)
							{
								C_BaseEntity* pEntity = (C_BaseEntity*)element->GetPointer();
								if (pEntity != nullptr)
								{
									// TODO: don't repeat this code

									//ImGuizmo::SetID(g_DirectorsCutSystem.elementIndex);

									float modelMatrix[16];
									Vector pos = pEntity->GetAbsOrigin();
									QAngle ang = pEntity->GetAbsAngles();
									float s = 1.f;

									if (element->GetType() == DAG_MODEL)
									{
										CModelElement* modelDag = (CModelElement*)element->GetPointer();
										if (modelDag != nullptr)
										{
											if (modelDag->IsRagdoll() && g_DirectorsCutSystem.boneIndex >= 0)
											{
												modelDag->m_pRagdoll->GetElement(g_DirectorsCutSystem.boneIndex)->GetPosition(&pos, &ang);
											}
											else if (g_DirectorsCutSystem.nextPoseIndex >= 0)
											{
												pos = g_DirectorsCutSystem.poseBoneOrigin;
												ang = g_DirectorsCutSystem.poseBoneAngles;
											}
										}
									}

									static float translation[3], rotation[3], scale[3];

									// Add offset (pivot) to translation
									pos -= g_DirectorsCutSystem.pivot;

									// Y up to Z up - pos is Z up, translation is Y up
									translation[0] = -pos.y;
									translation[1] = pos.z;
									translation[2] = -pos.x;

									rotation[0] = -ang.x;
									rotation[1] = ang.y;
									rotation[2] = -ang.z;

									scale[0] = s;
									scale[1] = s;
									scale[2] = s;

									ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, modelMatrix);
									float deltaMatrix[16];
									float deltaTranslation[3], deltaRotation[3], deltaScale[3];
									ImGuizmo::Manipulate(g_DirectorsCutSystem.cameraView, g_DirectorsCutSystem.cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, modelMatrix, deltaMatrix, (g_DirectorsCutSystem.useSnap ? g_DirectorsCutSystem.snap : NULL), NULL, NULL);
									ImGuizmo::DecomposeMatrixToComponents(deltaMatrix, deltaTranslation, deltaRotation, deltaScale);
									ImGuizmo::DecomposeMatrixToComponents(modelMatrix, translation, rotation, scale);
									// Revert Z up
									float translation2[3];
									translation2[0] = -deltaTranslation[2];
									translation2[1] = -deltaTranslation[0];
									translation2[2] = deltaTranslation[1];
									float rotation2[3];
									rotation2[0] = -deltaRotation[0];
									rotation2[1] = deltaRotation[1];
									rotation2[2] = -deltaRotation[2];
									// Rotating will not allow you to set transform due to issues.
									if (rotation2[0] != 0 || rotation2[1] != 0 || rotation2[2] != 0)
									{
										g_DirectorsCutSystem.deltaAngles = QAngle(rotation2[0], rotation2[1], rotation2[2]);
									}
									else if (translation2[0] != 0 || translation2[1] != 0 || translation2[2] != 0)
									{
										g_DirectorsCutSystem.deltaOrigin = Vector(translation2[0], translation2[1], translation2[2]);
									}
								}
							}
						}
					}

					// ctrl+click
					if (!g_DirectorsCutSystem.gotInput && g_DirectorsCutSystem.selecting && g_DirectorsCutSystem.hoveringInfo[0] >= 0)
					{
						// draw a "hint" text box at the mouse cursor
						ImGui::SetNextWindowPos(ImVec2(io.MousePos.x + 8, io.MousePos.y + 8));
						ImGui::Begin("Hint", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize);

						for (int i = 0; i < g_DirectorsCutSystem.elements.Count(); i++)
						{
							CElementPointer* element = g_DirectorsCutSystem.elements[i];
							if (element != nullptr)
							{
								if (i == g_DirectorsCutSystem.hoveringInfo[0])
								{
									char modelNameAndIndex[CHAR_MAX];
									sprintf(modelNameAndIndex, "%d", i);

									if (element->name != nullptr && element->name[0] != '\0')
									{
										sprintf(modelNameAndIndex, "%s (%s)", modelNameAndIndex, element->name);
									}
									
									DAG_ type = element->GetType();
									void* pointer = element->GetPointer();

									switch (type)
									{
									case DAG_MODEL:
									{
										CModelElement* model = (CModelElement*)pointer;
										if (model != nullptr)
											sprintf(modelNameAndIndex, "%s (%s)", modelNameAndIndex, STRING(model->GetModelPtr()->pszName()));
										break;
									}
									case DAG_LIGHT:
										sprintf(modelNameAndIndex, "%s (Light)", modelNameAndIndex);
										break;
									case DAG_CAMERA:
										sprintf(modelNameAndIndex, "%s (Camera)", modelNameAndIndex);
										break;
									case DAG_NONE:
										sprintf(modelNameAndIndex, "%s (Generic)", modelNameAndIndex);
										break;
									}
									ImGui::Text(modelNameAndIndex);
									if(type == DAG_MODEL && (g_DirectorsCutSystem.hoveringInfo[1] >= 0 || g_DirectorsCutSystem.hoveringInfo[2] >= 0))
									{
										CModelElement* modelDag = (CModelElement*)pointer;
										if (modelDag != nullptr)
										{
											CStudioHdr* modelPtr = modelDag->GetModelPtr();
											if (modelPtr != nullptr)
											{
												for(int k = 0; k < modelPtr->numbones(); k++)
												{
													mstudiobone_t* bone = modelPtr->pBone(k);
													if (bone != nullptr)
													{
														char boneName[CHAR_MAX];
														sprintf(boneName, "%d (%s)", k, bone->pszName());
														
														if (k == g_DirectorsCutSystem.hoveringInfo[2])
														{
															ImGui::Separator();
															ImGui::Text(boneName);
															break;
														}
													}
												}
											}
										}
									}
									break;
								}
							}
						}

						ImGui::End();
					}
				}
			}
			else if (g_DirectorsCutSystem.mainMenu)
			{
				// Display main menu
				ImGui::Begin("Directors Cut", NULL, ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("Welcome to Directors Cut!");
				ImGui::Text("Please load a map to begin.");
				ImGui::Separator();
				if(ImGui::Button("Load Map"))
				{
					// TODO: make this not so cluttered
					char szFile[MAX_PATH] = { 0 };
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = NULL;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Binary Space Partition (*.bsp)\0*.bsp\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
					// pull from exe path (wapi)
					char exePath[MAX_PATH];
					GetModuleFileName(NULL, exePath, MAX_PATH);
					char* exePathEnd = strrchr(exePath, '\\');
					*exePathEnd = '\0';
					ofn.lpstrInitialDir = exePath;
					if (GetOpenFileName(&ofn))
					{
						// Get file name, no path
						char* fileName = strrchr(ofn.lpstrFile, '\\');
						if (fileName != nullptr)
							fileName++;
						else
							fileName = ofn.lpstrFile;
						// call client cmd to load map
						char cmd[CHAR_MAX];
						sprintf(cmd, "map %s", fileName);
						engine->ClientCmd(cmd);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Discord"))
				{
					ShellExecute(0, 0, "https://discord.gg/3X3teNecWs", 0, 0, SW_SHOW);
				}
#if DX_UNIVERSE > 2
				ImGui::SameLine();
				if (ImGui::Button("GitHub"))
				{
					ShellExecute(0, 0, "https://github.com/SFMDX/DirectorsCut-Swarm", 0, 0, SW_SHOW);
				}
#endif
				ImGui::SameLine();
				if (ImGui::Button("Twitter"))
				{
					ShellExecute(0, 0, "https://twitter.com/SFMDirectorsCut", 0, 0, SW_SHOW);
				}
				ImGui::Separator();
				// Origin of build
				ImGui::Text("%s: %s", DX_UNIVERSE_TEXT, DX_UNIVERSE_ORIGIN);
				// Version of build
				ImGui::Text("Version: v%s", g_DirectorsCutSystem.directorcut_version.GetVersion());
				ImGui::End();
			}
			// Tutorial window
			if (g_DirectorsCutSystem.windowVisibilities[3])
			{
				// Set size
				ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
				ImGui::Begin("Tutorial", &g_DirectorsCutSystem.windowVisibilities[3]);

				// Left scrollable list for sections
				ImGui::BeginChild("Sections", ImVec2(150, 0), true);
				for (int i = 0; i < g_DirectorsCutSystem.tutorialSections.Count(); i++)
				{
					if (ImGui::Selectable(g_DirectorsCutSystem.tutorialSections[i]->GetName(), g_DirectorsCutSystem.tutorialSectionIndex == i))
					{
						g_DirectorsCutSystem.tutorialSectionIndex = i;
					}
				}
				ImGui::EndChild();

				// Right scrollable list for inner text
				ImGui::SameLine();
				ImGui::BeginChild("Text", ImVec2(0, 0), true);
				// Enable word wrap
				ImGui::PushTextWrapPos(0.0f);
				ImGui::Text(g_DirectorsCutSystem.tutorialSections[g_DirectorsCutSystem.tutorialSectionIndex]->GetInnerText());
				ImGui::PopTextWrapPos();
				ImGui::EndChild();

				ImGui::End();
			}
		}

		ImGui::PopFont();
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
	trampEndScene(p_pDevice);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
	// This hook redirects input to ImGui
	if (cvar_imgui_enabled.GetBool() && !engine->IsPaused())
	{
		ImGuiIO& io = ImGui::GetIO();
		LRESULT proc = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		if (proc || io.WantCaptureMouse || io.WantCaptureKeyboard)
		{
			return true;
		}
	}
	// TODO: Should we return here if ImGui capture input or?
	return CallWindowProc(ogWndProc, hWnd, msg, wParam, lParam);
}

void CDirectorsCutSystem::SetupEngineView(Vector& origin, QAngle& angles, float& fov)
{
	if (levelInit && cvar_imgui_enabled.GetBool() && !engine->IsPaused())
	{
		fov = this->fov;
		matrix_t_dx cameraDxMatrix = *(matrix_t_dx*)cameraView;
		matrix3x4_t cameraVMatrix;

		// Convert ImGuizmo matrix to matrix3x4_t
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				cameraVMatrix[i][j] = cameraDxMatrix.m[i][j];
			}
		}

		// Apply matrix to angles
		MatrixTranspose(cameraVMatrix, cameraVMatrix);
		MatrixAngles(cameraVMatrix, engineAngles);

		// TODO: Fix bad matrix angles
		if (engineAngles.y == 180)
		{
			angles.x = engineAngles.z;
			angles.y = -engineAngles.x;
			angles.z = engineAngles.y;
		}
		else
		{
			angles.x = -engineAngles.z;
			angles.y = engineAngles.x;
			angles.z = -engineAngles.y;
		}

		// Rotate origin around pivot point with distance
		Vector pivot = this->pivot;
		float distance = this->distance;
		Vector forward, right, up;
		AngleVectors(angles, &forward, &right, &up);
		engineOrigin = pivot - (forward * distance);
		origin = engineOrigin;

		// Lighting editor redundancy
		GetLightingEditor()->SetEditorView(&origin, &angles);
	}
}


void CDirectorsCutSystem::SetDefaultSettings()
{
	// set default values
	sprintf(modelName, "models/infected/common_male01.mdl");
	sprintf(lightTexture, "effects/flashlight001");
	bool newWindowVisibilities[4] = {
		true, true, true, true
	};
	for (int i = 0; i < 4; i++)
	{
		windowVisibilities[i] = newWindowVisibilities[i];
	}
	float newCameraMatrix[16] =
	{ 1.f, 0.f, 0.f, 0.f,
	  0.f, 1.f, 0.f, 0.f,
	  0.f, 0.f, 1.f, 0.f,
	  0.f, 0.f, 0.f, 1.f };
	for (int i = 0; i < 16; i++)
	{
		cameraView[i] = newCameraMatrix[i];
	}
	float newIdentityMatrix[16] =
	{ 1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f };
	for (int i = 0; i < 16; i++)
	{
		identityMatrix[i] = newIdentityMatrix[i];
	}
	float newSnap[3] = { 1.f, 1.f, 1.f };
	for (int i = 0; i < 3; i++)
	{
		snap[i] = newSnap[i];
	}
	int newHoveringInfo[3] = { -1, -1, -1 };
	for (int i = 0; i < 3; i++)
	{
		hoveringInfo[i] = newHoveringInfo[i];
	}
	distance = 100.f;
	fov = 93;
	fovAdjustment = 2;
	playerFov;
	camYAngle = 165.f / 180.f * M_PI_F;
	camXAngle = 32.f / 180.f * M_PI_F;
	gridSize = 500.f;
	currentTimeScale = 1.f;
	timeScale = 1.f;
	elementIndex = -1;
	nextElementIndex = -1;
	boneIndex = -1;
	poseIndex = -1;
	nextPoseIndex = -1;
	flexIndex = -1;
	operation = 2;
	oldOperation = 2;
	hoveringInfo[3];
	useSnap = false;
	orthographic = false;
	selecting = false;
	justSetPivot = false;
	pivotMode = false;
	spawnAtPivot = false;
	inspectorDocked = true;
	savedOnce = false;
}

void CDirectorsCutSystem::PostInit()
{
	SetDefaultSettings();
	LoadTutorialFromFile( "resource/directorscut/tutorial.vdf" );
	// Hook Direct3D in hl2.exe
	Msg("Director's Cut: Hooking Direct3D...\n");
	HWND hwnd = FindWindowA("Valve001", NULL);
	CDirect3DHook direct3DHook = GetDirect3DHook();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_Skin_DX::SetSkin(&ImGui::GetStyle());
	ImGui_ImplWin32_Init(hwnd);
	imguiActive = true;
	char* endSceneChar = reinterpret_cast<char*>(&EndScene);
	int hooked = direct3DHook.Hook(hwnd, endSceneChar);
	switch (hooked)
	{
		case 0: // No error
			ogWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
			Msg("Director's Cut: System loaded\n");
			break;
		case 1: // Already hooked
			Msg("Director's Cut: System already hooked\n");
			break;
		case 2: // Unable to hook endscene
			Msg("Director's Cut: Unable to hook endscene\n");
			break;
		case 4: // Invalid HWND
			Msg("Director's Cut: System failed to load - Invalid HWND\n");
			break;
		case 5: // Invalid device table
			Msg("Director's Cut: System failed to load - Invalid device table\n");
			break;
		case 6: // Could not find Direct3D system
			Msg("Director's Cut: System failed to load - Could not find Direct3D system\n");
			break;
		case 7: // Failed to create Direct3D device
			Msg("Director's Cut: System failed to load - Failed to create Direct3D device\n");
			break;
		case 3: // Future error
		default: // Unknown error
			Msg("Director's Cut: System failed to load - Unknown error\n");
			break;
	}
	// Unload Dear ImGui on nonzero
	if (hooked != 0)
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		imguiActive = false;
	}
}

void CDirectorsCutSystem::Shutdown()
{
	if (imguiActive)
	{
		// Unload Dear ImGui
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		imguiActive = false;
	}
	// Unhook
	CDirect3DHook direct3DHook = GetDirect3DHook();
	direct3DHook.Unhook();
	LevelShutdownPreEntity();
}

void CDirectorsCutSystem::LevelInitPostEntity()
{
	levelInit = true;
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer != nullptr)
	{
		// noclip
		if (pPlayer->GetMoveType() != MOVETYPE_NOCLIP)
		{
			// send noclip command to server
			engine->ClientCmd_Unrestricted("noclip");
		}
	}
	// Call clientcmd to unbind F1
	// I cannot find out why it defaults to "buildcubemap;quit"
	// Which is really annoying, so it'll just be overridden here
	engine->ClientCmd_Unrestricted("unbind F1");
}

void CDirectorsCutSystem::LevelShutdownPreEntity()
{
	levelInit = false;
	// loop g_DirectorsCutSystem.dags to remove all entities
	for (int i = 0; i < g_DirectorsCutSystem.elements.Count(); i++)
	{
		delete g_DirectorsCutSystem.elements[i];
	}
	// clear list
	g_DirectorsCutSystem.elements.RemoveAll();
}

void CDirectorsCutSystem::Update(float frametime)
{
	bool newState = (cvar_imgui_enabled.GetBool() && !engine->IsPaused() && levelInit);
	if (cursorState != newState)
	{
		cursorState = newState;
		// HACK: Using the light editor to unlock the cursor
		// Using this method causes the light editor to interfere with imgui
		// This can be fixed with an invisible vgui panel that swallows all input
		// Instead of using the light editor as a proxy for the same thing
		ConVar *var = cvar->FindVar("deferred_lighteditor_show");
		if (var != nullptr)
		{
			var->SetValue(newState);
		}
	}
	// only perform DX updates when appropriate
	if(newState)
	{
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if (pPlayer != nullptr)
		{
			clienttools->GetLocalPlayerEyePosition(playerOrigin, playerAngles, playerFov);
		}
		if (timeScale != currentTimeScale)
		{
			ConVar* varTimeScale = cvar->FindVar("host_timescale");
			varTimeScale->SetValue(timeScale);
			currentTimeScale = timeScale;
		}
		if (elementIndex > -1)
		{
			if (elementIndex < elements.Count())
			{
				CElementPointer* element = elements[elementIndex];
				if (element != nullptr)
				{
					C_BaseEntity* pEntity = (C_BaseEntity*)element->GetPointer();
					if (pEntity != nullptr)
					{
						bool shouldMoveGeneric = true;
						if (element->GetType() == DAG_MODEL)
						{
							CModelElement* modelDag = (CModelElement*)element->GetPointer();
							if (modelDag != nullptr)
							{
								if (nextPoseIndex >= 0)
								{
									shouldMoveGeneric = false;
									int nextPoseIndexTrue = nextPoseIndex;
									// pretend that the bone is in world space (it isn't)
									modelDag->PushAllowBoneAccess(true, true, "DirectorsCut");
									Vector worldVec;
									QAngle worldAng;
									MatrixAngles(modelDag->GetBoneForWrite(nextPoseIndexTrue), worldAng, worldVec);
									poseBoneOrigin = worldVec + modelDag->posadds[nextPoseIndexTrue];
									poseBoneAngles = worldAng + modelDag->anglehelper[nextPoseIndexTrue];
									if (deltaAngles.x != 0.f || deltaAngles.y != 0.f || deltaAngles.z != 0.f || deltaOrigin.x != 0.f || deltaOrigin.y != 0.f || deltaOrigin.z != 0.f)
									{
										// apply delta to pose
										poseBoneAngles.x += -deltaAngles.z;
										poseBoneAngles.y += deltaAngles.x;
										poseBoneAngles.z += deltaAngles.y;
										poseBoneOrigin += deltaOrigin;
										// remove world space for final pose
										Vector localVec = poseBoneOrigin - worldVec;
										QAngle localAng = poseBoneAngles - worldAng;
										modelDag->PoseBones(nextPoseIndexTrue, localVec, localAng);
									}
									modelDag->PopBoneAccess("DirectorsCut");
								}
							}
						}
						if(shouldMoveGeneric && (deltaAngles.x != 0.f || deltaAngles.y != 0.f || deltaAngles.z != 0.f || deltaOrigin.x != 0.f || deltaOrigin.y != 0.f || deltaOrigin.z != 0.f))
						{
							// apply delta to entity
							Vector origin = pEntity->GetAbsOrigin();
							QAngle angles = pEntity->GetAbsAngles();
							angles += deltaAngles;
							origin += deltaOrigin;
							pEntity->SetAbsOrigin(origin);
							pEntity->SetAbsAngles(angles);
						}
						deltaOrigin = Vector(0.f, 0.f, 0.f);
						deltaAngles = QAngle(0.f, 0.f, 0.f);
					}
				}
			}
			if (selecting)
			{
				if (operation != -2)
				{
					oldOperation = operation;
					operation = -2;
				}
				// holding down CTRL (selecting) should allow the user to select individual objects
				float vector = 0.25f;
				Vector boundMin = Vector(-vector, -vector, -vector);
				Vector boundMax = Vector(vector, vector, vector);
				int colorR = 196;
				int colorG = 107;
				int colorB = 174;
				int colorA = 255;
				int colorR_hover = 255;
				int colorG_hover = 255;
				int colorB_hover = 255;
				int colorA_hover = 255;
				int colorR_selected = 96;
				int colorG_selected = 96;
				int colorB_selected = 96;
				int colorA_selected = 255;
				int colorR_frozen = 0;
				int colorG_frozen = 255;
				int colorB_frozen = 255;
				int colorA_frozen = 255;
				int mouseX, mouseY;
				bool hovering = false;
				input->GetFullscreenMousePos(&mouseX, &mouseY);
				if (elementIndex <= -1)
					return;
				int modelBones = 0;
				if (boneIndex >= 0)
					modelBones = 1;
				else if (nextPoseIndex >= 0)
					modelBones = 2;
				// get engine mouse click
				bool clicked = vgui::input()->IsMouseDown( MOUSE_LEFT );
				// loop all models
				for (int i = 0; i < elements.Count(); i++)
				{
					CElementPointer* element = elements[i];
					if (element != nullptr)
					{
						C_BaseEntity* pEntity = (C_BaseEntity*)element->GetPointer();
						if (pEntity != nullptr)
						{
							if (modelBones == 0)
							{
								char modelNameAndIndex[CHAR_MAX];
								sprintf(modelNameAndIndex, "%d", i);
								// check element->name, if it's not empty, use that instead
								if (element->name != nullptr && element->name[0] != '\0')
								{
									sprintf(modelNameAndIndex, "%s (%s)", modelNameAndIndex, element->name);
								}

								switch (element->GetType())
								{
								case DAG_MODEL:
								{
									CModelElement* model = (CModelElement*)element->GetPointer();
									if (model != nullptr)
										sprintf(modelNameAndIndex, "%s (%s)", modelNameAndIndex, STRING(model->GetModelPtr()->pszName()));
									break;
								}
								case DAG_LIGHT:
									sprintf(modelNameAndIndex, "%s (Light)", modelNameAndIndex);
									break;
								case DAG_CAMERA:
									sprintf(modelNameAndIndex, "%s (Camera)", modelNameAndIndex);
									break;
								case DAG_NONE:
									sprintf(modelNameAndIndex, "%s (Generic)", modelNameAndIndex);
									break;
								}

								if (i == elementIndex)
								{
									//NDebugOverlay::Box(model->GetAbsOrigin(), boundMin, boundMax, colorR_selected, colorG_selected, colorB_selected, colorA_selected, length);
									NDebugOverlay::EntityTextAtPosition(pEntity->GetAbsOrigin(), 0, modelNameAndIndex, frametime, colorR_selected, colorG_selected, colorB_selected, colorA_selected);
								}
								else
								{
									// is mouse in 3d bounds
									Vector screenPos;
									ScreenTransform(pEntity->GetAbsOrigin(), screenPos);
									// vector is normalized from -1 to 1, so we have to normalize mouseX and mouseY
									float mouseXNorm = (mouseX / (float)ScreenWidth()) * 2 - 1;
									float mouseYNorm = ((mouseY / (float)ScreenHeight()) * 2 - 1) * -1;
									// instead of directly computing boundMin and boundMax in 3d space, we'll use vector as an estimate
									if (!hovering && mouseXNorm > screenPos.x - (vector * 0.1) && mouseXNorm < screenPos.x + (vector * 0.1) && mouseYNorm > screenPos.y - (vector * 0.1) && mouseYNorm < screenPos.y + (vector * 0.1))
									{
										//NDebugOverlay::Box(model->GetAbsOrigin(), boundMin, boundMax, colorR_hover, colorG_hover, colorB_hover, colorA_hover, length);
										NDebugOverlay::EntityTextAtPosition(pEntity->GetAbsOrigin(), 0, modelNameAndIndex, frametime, colorR_hover, colorG_hover, colorB_hover, colorA_hover);
										// mouse input
										if (clicked)
										{
											vgui::surface()->PlaySound("common/wpn_select.wav");
											// select model
											clicked = false;
											g_DirectorsCutSystem.deltaOrigin = Vector(0, 0, 0);
											g_DirectorsCutSystem.deltaAngles = QAngle(0, 0, 0);
											elementIndex = i;
										}
										hoveringInfo[0] = i;
										hoveringInfo[1] = -1;
										hoveringInfo[2] = -1;
										hovering = true;
									}
									else
									{
										//NDebugOverlay::Box(model->GetAbsOrigin(), boundMin, boundMax, colorR, colorG, colorB, colorA, length);
										NDebugOverlay::EntityTextAtPosition(pEntity->GetAbsOrigin(), 0, modelNameAndIndex, frametime, colorR, colorG, colorB, colorA);
									}
								}
							}
							else if (element->GetType() == DAG_MODEL)
							{
								// draw model locations and names
								CModelElement* model = (CModelElement*)element->GetPointer();
								if (model != nullptr)
								{
									// use the current model
									CStudioHdr* modelPtr = model->GetModelPtr();
									if (modelPtr != nullptr)
									{
										if (modelBones == 2)
										{
											model->PushAllowBoneAccess(true, true, "DirectorsCut_PostRender");
											// loop all pose bones
											for (int k = 0; k < modelPtr->numbones(); k++)
											{
												mstudiobone_t* bone = modelPtr->pBone(k);
												if (bone != nullptr)
												{
													// draw pose bone locations and names
													Vector worldVec;
													QAngle worldAng;
													MatrixAngles(model->GetBoneForWrite(k), worldAng, worldVec);
													char nameAndIndex[CHAR_MAX];
													sprintf(nameAndIndex, "%d", k); //bone->pszName());
													//Msg("%s (len: %d)\n", nameAndIndex, strlen(nameAndIndex));
													// is mouse in 3d bounds
													Vector screenPos;
													ScreenTransform(worldVec + model->posadds[k], screenPos);
													// vector is normalized from -1 to 1, so we have to normalize mouseX and mouseY
													float mouseXNorm = (mouseX / (float)ScreenWidth()) * 2 - 1;
													float mouseYNorm = ((mouseY / (float)ScreenHeight()) * 2 - 1) * -1;
													// instead of directly computing boundMin and boundMax in 3d space, we'll use vector as an estimate
													if (k == nextPoseIndex)
													{
														//NDebugOverlay::Box(worldVec + model->posadds[i], boundMin, boundMax, colorR_selected, colorG_selected, colorB_selected, colorA_selected, length);
														NDebugOverlay::EntityTextAtPosition(worldVec + model->posadds[i], 0, nameAndIndex, frametime, colorR_selected, colorG_selected, colorB_selected, colorA_selected);
													}
													else
													{
														if (!hovering && mouseXNorm > screenPos.x - (vector * 0.1) && mouseXNorm < screenPos.x + (vector * 0.1) && mouseYNorm > screenPos.y - (vector * 0.1) && mouseYNorm < screenPos.y + (vector * 0.1))
														{
															//NDebugOverlay::Box(worldVec + model->posadds[i], boundMin, boundMax, colorR_hover, colorG_hover, colorB_hover, colorA_hover, length);
															NDebugOverlay::EntityTextAtPosition(worldVec + model->posadds[k], 0, nameAndIndex, frametime, colorR_hover, colorG_hover, colorB_hover, colorA_hover);
															// forced to use imgui as it swallows mouse input
															if (clicked)
															{
																vgui::surface()->PlaySound("common/wpn_select.wav");
																// select model
																clicked = false;
																g_DirectorsCutSystem.deltaOrigin = Vector(0, 0, 0);
																g_DirectorsCutSystem.deltaAngles = QAngle(0, 0, 0);
																nextPoseIndex = k;
															}
															hoveringInfo[0] = i;
															hoveringInfo[1] = -1;
															hoveringInfo[2] = k;
															hovering = true;
														}
														else
														{
															//NDebugOverlay::Box(worldVec + model->posadds[i], boundMin, boundMax, colorR, colorG, colorB, colorA, length);
															NDebugOverlay::EntityTextAtPosition(worldVec + model->posadds[k], 0, nameAndIndex, frametime, colorR, colorG, colorB, colorA);
														}
													}
												}
											}
											model->PopBoneAccess("DirectorsCut_PostRender");
											break;
										}
									}
								}
							}
						}
					}
				}
				if (!hovering)
				{
					hoveringInfo[0] = -1;
					hoveringInfo[1] = -1;
					hoveringInfo[2] = -1;
				}
			}
			else if (operation == -2)
			{
				operation = oldOperation;
			}
			input->ClearInputButton(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_RELOAD | IN_USE | IN_CANCEL | IN_LEFT | IN_RIGHT | IN_SCORE | IN_SPEED | IN_WALK | IN_ZOOM | IN_BULLRUSH | IN_RUN);
			// set pose bone index
			// TODO: make sure bone is in range
			if (poseIndex != nextPoseIndex && nextPoseIndex >= 0 && elementIndex >= 0 && elementIndex < elements.Count())
			{
				CElementPointer* element = elements[elementIndex];
				if (element != nullptr)
				{
					C_BaseEntity* pEntity = (C_BaseEntity*)element->GetPointer();
					if (pEntity != nullptr)
					{
						if (element->GetType() == DAG_MODEL)
						{
							CModelElement* pEntity2 = (CModelElement*)element->GetPointer();
							if (pEntity2 != nullptr)
							{
								pEntity2->GetBoneForWrite(nextPoseIndex);
								poseIndex = nextPoseIndex;
							}
						}
					}
				}
			}
		}

		// update all light elements
		for (int i = 0; i < elements.Count(); i++)
		{
			CElementPointer* element = elements[i];
			if (element != nullptr)
			{
				C_BaseEntity* pEntity = (C_BaseEntity*)element->GetPointer();
				if (pEntity != nullptr)
				{
					if (element->GetType() == DAG_LIGHT)
					{
						CLightElement* pEntity2 = (CLightElement*)element->GetPointer();
						if (pEntity2 != nullptr)
						{
							// get dir, right, and up from angles
							QAngle angles = pEntity2->GetAbsAngles();
							Vector dir, right, up;
							AngleVectors(angles, &dir, &right, &up);
							pEntity2->UpdateLight(pEntity2->GetAbsOrigin(), dir, right, up, 1000);
						}
					}
				}
			}
		}
	}
}

void FromKeyToBoolArray(KeyValues* parent, char* key, int max, bool* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		KeyValues* value = child->FindKey(key);
		array[i] = value->GetBool();
	}
}

void FromBoolArrayToKey(KeyValues* parent, char* key, int max, bool* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		child->SetBool(key, array[i]);
	}
}

void FromKeyToIntArray(KeyValues* parent, char* key, int max, int* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		KeyValues* value = child->FindKey(key);
		array[i] = value->GetInt();
	}
}

void FromIntArrayToKey(KeyValues* parent, char* key, int max, int* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		child->SetInt(key, array[i]);
	}
}

void FromKeyToFloatArray(KeyValues* parent, char* key, int max, float* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		KeyValues* value = child->FindKey(key, true);
		array[i] = value->GetFloat();
	}
}

void FromFloatArrayToKey(KeyValues* parent, char* key, int max, float* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		child->SetFloat(key, array[i]);
	}
}

void FromKeyToVector(KeyValues* parent, char* key, Vector &vector)
{
	KeyValues* child = parent->FindKey(key, true);
	vector.x = child->FindKey("x", true)->GetFloat();
	vector.y = child->FindKey("y", true)->GetFloat();
	vector.z = child->FindKey("z", true)->GetFloat();
}

void FromVectorToKey(KeyValues* parent, char* key, Vector vector)
{
	KeyValues* child = parent->FindKey(key, true);
	child->SetFloat("x", vector.x);
	child->SetFloat("y", vector.y);
	child->SetFloat("z", vector.z);
}

void FromKeyToVectorArray(KeyValues* parent, char* key, int max, Vector* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		FromKeyToVector(child, key, array[i]);
	}
}

void FromVectorArrayToKey(KeyValues* parent, char* key, int max, Vector* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		FromVectorToKey(child, key, array[i]);
	}
}

void FromKeyToAngles(KeyValues* parent, char* key, QAngle &vector)
{
	KeyValues* child = parent->FindKey(key, true);
	vector.x = child->FindKey("x", true)->GetFloat();
	vector.y = child->FindKey("y", true)->GetFloat();
	vector.z = child->FindKey("z", true)->GetFloat();
}

void FromAnglesToKey(KeyValues* parent, char* key, QAngle vector)
{
	KeyValues* child = parent->FindKey(key, true);
	child->SetFloat("x", vector.x);
	child->SetFloat("y", vector.y);
	child->SetFloat("z", vector.z);
}

void FromKeyToAnglesArray(KeyValues* parent, char* key, int max, QAngle* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		FromKeyToAngles(child, key, array[i]);
	}
}

void FromAnglesArrayToKey(KeyValues* parent, char* key, int max, QAngle* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		FromAnglesToKey(child, key, array[i]);
	}
}

void FromKeyToQuaternion(KeyValues* parent, char* key, Quaternion &vector)
{
	KeyValues* child = parent->FindKey(key, true);
	vector.x = child->FindKey("x", true)->GetFloat();
	vector.y = child->FindKey("y", true)->GetFloat();
	vector.z = child->FindKey("z", true)->GetFloat();
	vector.w = child->FindKey("w", true)->GetFloat();
}

void FromQuaternionToKey(KeyValues* parent, char* key, Quaternion vector)
{
	KeyValues* child = parent->FindKey(key, true);
	child->SetFloat("x", vector.x);
	child->SetFloat("y", vector.y);
	child->SetFloat("z", vector.z);
	child->SetFloat("w", vector.w);
}

void FromKeyToQuaternionArray(KeyValues* parent, char* key, int max, Quaternion* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		FromKeyToQuaternion(child, key, array[i]);
	}
}

void FromQuaternionArrayToKey(KeyValues* parent, char* key, int max, Quaternion* array)
{
	KeyValues* child = parent->FindKey(key, true);
	for (int i = 0; i < max; i++)
	{
		char key[CHAR_MAX];
		sprintf(key, "%d", i);
		FromQuaternionToKey(child, key, array[i]);
	}
}

void UnserializeKeyValuesDX(KeyValues* kv, bool append)
{
	// Check format type
	KeyValues* version = kv->FindKey("version", true);
	if (version->GetInt("major") != g_DirectorsCutSystem.directorcut_version.m_major)
	{
		Warning("DirectorsCut: UnserializeKeyValuesDX: Version mismatch! Expected major %d, got %d\n", g_DirectorsCutSystem.directorcut_version.m_major, version->GetInt("major"));
		return;
	}
	if(version->GetInt("minor") != g_DirectorsCutSystem.directorcut_version.m_minor)
	{
		Warning("DirectorsCut: UnserializeKeyValuesDX: Version mismatch! Expected minor %d, got %d\n", g_DirectorsCutSystem.directorcut_version.m_minor, version->GetInt("minor"));
		return;
	}
	// patches will be ignored
	// Load session	data
	KeyValues* elements = kv->FindKey("session", true);
	if (elements != nullptr)
	{
		if (!append)
		{
			for (int i = 0; i < g_DirectorsCutSystem.elements.Count(); i++)
			{
				delete g_DirectorsCutSystem.elements[i];
			}
			g_DirectorsCutSystem.elements.RemoveAll();
		}
		for (KeyValues* element = elements->GetFirstSubKey(); element != nullptr; element = element->GetNextKey())
		{
			// Get element type
			int type = element->GetInt("type");
			DAG_ dagType = DAG_NONE;
			switch (type)
			{
			case 0:
				dagType = DAG_MODEL;
				break;
			case 1:
				dagType = DAG_LIGHT;
				break;
			case 2:
				dagType = DAG_CAMERA;
				break;
			}
			// Get element name
			char* name = (char*)element->GetString("name");
			// Get element data
			KeyValues* data = element->FindKey("data", true);
			// Create element
			CElementPointer* elementPtr = CreateElement(false, false, dagType, name, data);
			if (element == nullptr)
			{
				Msg("Director's Cut: Failed to create element %s\n", name);
				continue;
			}
			// Add element to list
			g_DirectorsCutSystem.elements.AddToTail(elementPtr);
		}
	}
	KeyValues* settings = kv->FindKey("settings", true);
	if(settings != nullptr)
	{
		FromKeyToBoolArray(settings, "windowVisibilities", 4, g_DirectorsCutSystem.windowVisibilities);
		FromKeyToFloatArray(settings, "cameraMatrix", 16, g_DirectorsCutSystem.cameraView);
		FromKeyToFloatArray(settings, "cameraProjection", 16, g_DirectorsCutSystem.cameraProjection);
		FromKeyToFloatArray(settings, "identityMatrix", 16, g_DirectorsCutSystem.identityMatrix);
		FromKeyToFloatArray(settings, "snap", 3, g_DirectorsCutSystem.snap);
		FromKeyToVector(settings, "pivot", g_DirectorsCutSystem.pivot);
		sprintf(g_DirectorsCutSystem.modelName, "%s", settings->GetString("modelName"));
		sprintf(g_DirectorsCutSystem.lightTexture, "%s", settings->GetString("lightTexture"));
		g_DirectorsCutSystem.operation = settings->GetInt("operation");
		g_DirectorsCutSystem.useSnap = settings->GetInt("useSnap");
		g_DirectorsCutSystem.orthographic = settings->GetInt("orthographic");
		g_DirectorsCutSystem.pivotMode = settings->GetInt("pivotMode");
		g_DirectorsCutSystem.spawnAtPivot = settings->GetInt("spawnAtPivot");
		g_DirectorsCutSystem.inspectorDocked = settings->GetInt("inspectorDocked");
		g_DirectorsCutSystem.gotInput = settings->GetInt("gotInput");
		g_DirectorsCutSystem.distance = settings->GetFloat("distance");
		g_DirectorsCutSystem.fov = settings->GetFloat("fov");
		g_DirectorsCutSystem.fovAdjustment = settings->GetFloat("fovAdjustment");
		g_DirectorsCutSystem.playerFov = settings->GetFloat("playerFov");
		g_DirectorsCutSystem.camYAngle = settings->GetFloat("camYAngle");
		g_DirectorsCutSystem.camXAngle = settings->GetFloat("camXAngle");
		g_DirectorsCutSystem.gridSize = settings->GetFloat("gridSize");
		g_DirectorsCutSystem.currentTimeScale = settings->GetFloat("currentTimeScale");
		g_DirectorsCutSystem.timeScale = settings->GetFloat("timeScale");
	}
}

void LoadFromKeyValues(char* path, bool append)
{
	// Load file
	KeyValues* kv = new KeyValues("DirectorsCut");
	kv->LoadFromFile(g_pFullFileSystem, path, NULL);
	UnserializeKeyValuesDX(kv, append);
	kv->deleteThis();
}

KeyValues* SerializeKeyValuesDX()
{
	KeyValues* kv = new KeyValues("DirectorsCut");
	KeyValues* version = kv->FindKey("version", true);
	version->SetInt("major", g_DirectorsCutSystem.directorcut_version.m_major);
	version->SetInt("minor", g_DirectorsCutSystem.directorcut_version.m_minor);
	version->SetInt("patch", g_DirectorsCutSystem.directorcut_version.m_patch);
	// Save session data
	KeyValues* elements = kv->FindKey("session", true);
	for (int i = 0; i < g_DirectorsCutSystem.elements.Count(); i++)
	{
		// Get element
		CElementPointer* element = g_DirectorsCutSystem.elements[i];
		KeyValues* elementKey = elements->CreateNewKey();
		int type = -1;
		switch (element->GetType())
		{
		case DAG_MODEL:
			type = 0;
			break;
		case DAG_LIGHT:
			type = 1;
			break;
		case DAG_CAMERA:
			type = 2;
			break;
		}
		elementKey->SetInt("type", type);
		elementKey->SetString("name", element->name);
		KeyValues* data = elementKey->FindKey("data", true);
		C_BaseEntity* genericEntity = (C_BaseEntity*)element->GetPointer();
		if(genericEntity == nullptr)
			continue;
		Vector origin = genericEntity->GetAbsOrigin();
		data->SetFloat("pivotX", origin.x);
		data->SetFloat("pivotY", origin.y);
		data->SetFloat("pivotZ", origin.z);
		FromAnglesToKey(data, "angles", (QAngle)genericEntity->GetAbsAngles());
		// Save element data
		switch(element->GetType())
		{
		case DAG_MODEL:
			{
				CModelElement* model = (CModelElement*)element->GetPointer();
				if(model == nullptr)
					continue;
				data->SetString("modelName", STRING(model->GetModelPtr()->pszName()));
				CStudioHdr* modelPtr = model->GetModelPtr();
				if (modelPtr == nullptr)
					continue;
				FromVectorArrayToKey(data, "posadds", modelPtr->numbones(), model->posadds);
				FromAnglesArrayToKey(data, "anglehelper", modelPtr->numbones(), model->anglehelper);
				FromFloatArrayToKey(data, "forcedFlexes", modelPtr->numflexcontrollers(), model->forcedFlexes);
				break;
			}
		case DAG_LIGHT:
			{
				CLightElement* model = (CLightElement*)element->GetPointer();
				if (model == nullptr)
					continue;
				// TODO: lights
				data->SetString("lightTexture", "");
				break;
			}
		case DAG_CAMERA:
			{
				CCameraElement* model = (CCameraElement*)element->GetPointer();
				if (model == nullptr)
					continue;
				// TODO: cameras
				break;
			}
		}
	}
	KeyValues* settings = kv->FindKey("settings", true);
	FromBoolArrayToKey(settings, "windowVisibilities", 2, g_DirectorsCutSystem.windowVisibilities);
	FromFloatArrayToKey(settings, "cameraMatrix", 16, g_DirectorsCutSystem.cameraView);
	FromFloatArrayToKey(settings, "cameraProjection", 16, g_DirectorsCutSystem.cameraProjection);
	FromFloatArrayToKey(settings, "identityMatrix", 16, g_DirectorsCutSystem.identityMatrix);
	FromFloatArrayToKey(settings, "snap", 3, g_DirectorsCutSystem.snap);
	FromVectorToKey(settings, "pivot", g_DirectorsCutSystem.pivot);
	settings->SetString("modelName", g_DirectorsCutSystem.modelName);
	settings->SetString("lightTexture", g_DirectorsCutSystem.lightTexture);
	settings->SetInt("operation", g_DirectorsCutSystem.operation);
	settings->SetInt("useSnap", g_DirectorsCutSystem.useSnap);
	settings->SetInt("orthographic", g_DirectorsCutSystem.orthographic);
	settings->SetInt("pivotMode", g_DirectorsCutSystem.pivotMode);
	settings->SetInt("spawnAtPivot", g_DirectorsCutSystem.spawnAtPivot);
	settings->SetInt("inspectorDocked", g_DirectorsCutSystem.inspectorDocked);
	settings->SetInt("gotInput", g_DirectorsCutSystem.gotInput);
	settings->SetFloat("distance", g_DirectorsCutSystem.distance);
	settings->SetFloat("fov", g_DirectorsCutSystem.fov);
	settings->SetFloat("fovAdjustment", g_DirectorsCutSystem.fovAdjustment);
	settings->SetFloat("playerFov", g_DirectorsCutSystem.playerFov);
	settings->SetFloat("camYAngle", g_DirectorsCutSystem.camYAngle);
	settings->SetFloat("camXAngle", g_DirectorsCutSystem.camXAngle);
	settings->SetFloat("gridSize", g_DirectorsCutSystem.gridSize);
	settings->SetFloat("currentTimeScale", g_DirectorsCutSystem.currentTimeScale);
	settings->SetFloat("timeScale", g_DirectorsCutSystem.timeScale);
	return kv;
}

void SaveToKeyValues(char* path)
{
	KeyValues* kv = SerializeKeyValuesDX();
	kv->SaveToFile(g_pFullFileSystem, path, "MOD");
	kv->deleteThis();
}

void ImportSFMSession(char* path, int frame)
{
	// unfinished SFM import code
	// there's a temporary "hack" to import sessions for now
	// use SFM SOCK (sfm bridge)
	DECLARE_DMX_CONTEXT();
	CDmxElement* DMX = (CDmxElement*)DMXAlloc(50000000);
	CUtlBuffer buf;
	if (UnserializeDMX(path, "MOD", false, &DMX))
	{
		
	}
}

CElementPointer* CreateElement(bool add, bool setIndex, DAG_ type)
{
	// using keyvalues here as parameters may differ between types
	// also futureproofs new element types
	KeyValues* kv = new KeyValues("DirectorsCutElement");

	switch (type)
	{
	case DAG_MODEL:
		kv->SetString("modelName", g_DirectorsCutSystem.modelName);
		break;
	case DAG_LIGHT:
		kv->SetString("lightTexture", g_DirectorsCutSystem.lightTexture);
		break;
	}

	kv->SetFloat("pivotX", g_DirectorsCutSystem.spawnAtPivot ? g_DirectorsCutSystem.pivot.x : 0);
	kv->SetFloat("pivotY", g_DirectorsCutSystem.spawnAtPivot ? g_DirectorsCutSystem.pivot.y : 0);
	kv->SetFloat("pivotZ", g_DirectorsCutSystem.spawnAtPivot ? g_DirectorsCutSystem.pivot.z : 0);

	CElementPointer* element = CreateElement(add, setIndex, type, "", kv);
	kv->deleteThis();
	if (element != nullptr)
		return element;
	return NULL;
}

CElementPointer* CreateElement(bool add, bool setIndex, DAG_ type, char* name, KeyValues* kv)
{
	CElementPointer* newElement = new CElementPointer(type, kv);
	if (newElement != nullptr)
	{
		sprintf(newElement->name, "%s", name);
		if (add)
		{
			g_DirectorsCutSystem.boneIndex = -1;
			g_DirectorsCutSystem.nextPoseIndex = -1;
			g_DirectorsCutSystem.elements.AddToTail(newElement);
			if (setIndex)
				g_DirectorsCutSystem.elementIndex = g_DirectorsCutSystem.elements.Count() - 1;
		}
		return newElement;
	}
	return nullptr;
}

void CDirectorsCutSystem::Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
{
	float temp, temp2, temp3, temp4;
	temp = 2.0f * znear;
	temp2 = right - left;
	temp3 = top - bottom;
	temp4 = zfar - znear;
	m16[0] = temp / temp2;
	m16[1] = 0.0;
	m16[2] = 0.0;
	m16[3] = 0.0;
	m16[4] = 0.0;
	m16[5] = temp / temp3;
	m16[6] = 0.0;
	m16[7] = 0.0;
	m16[8] = (right + left) / temp2;
	m16[9] = (top + bottom) / temp3;
	m16[10] = (-zfar - znear) / temp4;
	m16[11] = -1.0f;
	m16[12] = 0.0;
	m16[13] = 0.0;
	m16[14] = (-temp * zfar) / temp4;
	m16[15] = 0.0;
}

void CDirectorsCutSystem::OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, float* m16)
{
	m16[0] = 2 / (r - l);
	m16[1] = 0.0f;
	m16[2] = 0.0f;
	m16[3] = 0.0f;
	m16[4] = 0.0f;
	m16[5] = 2 / (t - b);
	m16[6] = 0.0f;
	m16[7] = 0.0f;
	m16[8] = 0.0f;
	m16[9] = 0.0f;
	m16[10] = 1.0f / (zf - zn);
	m16[11] = 0.0f;
	m16[12] = (l + r) / (l - r);
	m16[13] = (t + b) / (b - t);
	m16[14] = zn / (zn - zf);
	m16[15] = 1.0f;
}

void CDirectorsCutSystem::Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float* m16)
{
	float ymax, xmax;
	ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
	xmax = ymax * aspectRatio;
	this->Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
}

void Cross(const float* a, const float* b, float* r)
{
	r[0] = a[1] * b[2] - a[2] * b[1];
	r[1] = a[2] * b[0] - a[0] * b[2];
	r[2] = a[0] * b[1] - a[1] * b[0];
}

float Dot(const float* a, const float* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void Normalize(const float* a, float* r)
{
	float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
	r[0] = a[0] * il;
	r[1] = a[1] * il;
	r[2] = a[2] * il;
}

void CDirectorsCutSystem::LookAt(const float* eye, const float* at, const float* up, float* m16)
{
	float X[3], Y[3], Z[3], tmp[3];

	tmp[0] = eye[0] - at[0];
	tmp[1] = eye[1] - at[1];
	tmp[2] = eye[2] - at[2];
	Normalize(tmp, Z);
	Normalize(up, Y);

	Cross(Y, Z, tmp);
	Normalize(tmp, X);

	Cross(Z, X, tmp);
	Normalize(tmp, Y);

	m16[0] = X[0];
	m16[1] = Y[0];
	m16[2] = Z[0];
	m16[3] = 0.0f;
	m16[4] = X[1];
	m16[5] = Y[1];
	m16[6] = Z[1];
	m16[7] = 0.0f;
	m16[8] = X[2];
	m16[9] = Y[2];
	m16[10] = Z[2];
	m16[11] = 0.0f;
	m16[12] = -Dot(X, eye);
	m16[13] = -Dot(Y, eye);
	m16[14] = -Dot(Z, eye);
	m16[15] = 1.0f;
}

bool CDummyProxy::Init(IMaterial* pMaterial, KeyValues* pKeyValues)
{
	mat = pMaterial;
	return true;
};

IMaterial* CDummyProxy::GetMaterial()
{
	return mat;
}

bool CDummyProxyResultFloat::Init(IMaterial* pMaterial, KeyValues* pKeyValues)
{
	bool foundVar;

	char const* pResultVarName = pKeyValues->GetString("resultVar");
	if (!pResultVarName)
		return false;

	resultVar = pMaterial->FindVar(pResultVarName, &foundVar, false);
	if (!foundVar)
		return false;

	resultVar->SetFloatValue(0.f);

	mat = pMaterial;
	return true;
};

bool CDummyProxyResultFloatInverted::Init(IMaterial* pMaterial, KeyValues* pKeyValues)
{
	bool foundVar;

	char const* pResultVarName = pKeyValues->GetString("resultVar");
	if (!pResultVarName)
		return false;

	resultVar = pMaterial->FindVar(pResultVarName, &foundVar, false);
	if (!foundVar)
		return false;

	resultVar->SetFloatValue(1.f);
	
	mat = pMaterial;
	return true;
};

bool CDummyProxyResultRGB::Init(IMaterial* pMaterial, KeyValues* pKeyValues)
{
	bool foundVar;

	char const* pResultVarName = pKeyValues->GetString("resultVar");
	if (!pResultVarName)
		return false;

	resultVar = pMaterial->FindVar(pResultVarName, &foundVar, false);
	if (!foundVar)
		return false;

	resultVar->SetStringValue("[0 0 0]");

	mat = pMaterial;
	return true;
};

bool CDummyProxyResultRGBInverted::Init(IMaterial* pMaterial, KeyValues* pKeyValues)
{
	bool foundVar;

	char const* pResultVarName = pKeyValues->GetString("resultVar");
	if (!pResultVarName)
		return false;

	resultVar = pMaterial->FindVar(pResultVarName, &foundVar, false);
	if (!foundVar)
		return false;

	resultVar->SetStringValue("[1 1 1]");

	mat = pMaterial;
	return true;
};

void LoadTutorialFromFile(const char* filename)
{
	if(!filename)
		return;

	KeyValues* pKeyValues = new KeyValues("Tutorial");
	if (!pKeyValues->LoadFromFile(g_pFullFileSystem, filename, "MOD"))
	{
		pKeyValues->deleteThis();
		return;
	}

	KeyValues* pKeyValues2 = pKeyValues->GetFirstSubKey();
	while (pKeyValues2)
	{
		const char* pName = pKeyValues2->GetName();
		const int innerTextCount = pKeyValues2->GetInt("InnerTextCount");
		char* pInnerText = new char[8192];
		for (int i = 0; i < innerTextCount; i++)
		{
			char* pInnerTextChildKey = new char[1024];
			sprintf(pInnerTextChildKey, "InnerText%d", i);
			const char* pInnerTextChild = pKeyValues2->GetString(pInnerTextChildKey);
			if (pInnerTextChild)
			{
				// Merge with \n
				if (i == 0)
					sprintf(pInnerText, "%s", pInnerTextChild);
				else
					sprintf(pInnerText, "%s\n%s", pInnerText, pInnerTextChild);
			}
		}
		TutorialSection* section = new TutorialSection(pName, pInnerText);
		g_DirectorsCutSystem.tutorialSections.AddToTail(section);
		pKeyValues2 = pKeyValues2->GetNextKey();
	}

	pKeyValues->deleteThis();
}
