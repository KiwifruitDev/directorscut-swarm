//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DIRECTORSCUT_H_
#define _DIRECTORSCUT_H_

#include "igamesystem.h"
#include "physpropclientside.h"
#include "mathlib/vector.h"
#include "dag_entity.h"
#include <string>
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "imgui_public.h"
#include "tutorial.h"

// Version information
// Increment for each release
#define DX_VERSION_MAJOR 0
#define DX_VERSION_MINOR 2
#define DX_VERSION_PATCH 0

// These refer to where this build is being uploaded to
#define DX_UNIVERSE_PUBLIC 0
#define DX_UNIVERSE_BETA 1

// Text definitions for each universe
#define DX_UNIVERSE_PUBLIC_TEXT "Public"
#define DX_UNIVERSE_BETA_TEXT "Beta"

// Text origins for each universe
#define DX_UNIVERSE_PUBLIC_ORIGIN "Steam release"
#define DX_UNIVERSE_BETA_ORIGIN "For testing purposes only"

// Preprocessor definition helpers
#ifdef DX_UNIVERSE_SET_PUBLIC
#define DX_UNIVERSE DX_UNIVERSE_PUBLIC
#endif
#ifdef DX_UNIVERSE_SET_BETA
#define DX_UNIVERSE DX_UNIVERSE_BETA
#endif

#ifndef DX_UNIVERSE
#pragma message("DX_UNIVERSE is not defined, defaulting to DX_UNIVERSE_BETA.")
#define DX_UNIVERSE DX_UNIVERSE_BETA
#endif

// Set the universe text
#if DX_UNIVERSE == DX_UNIVERSE_PUBLIC
#define DX_UNIVERSE_TEXT DX_UNIVERSE_PUBLIC_TEXT
#define DX_UNIVERSE_ORIGIN DX_UNIVERSE_PUBLIC_ORIGIN
#endif
#if DX_UNIVERSE == DX_UNIVERSE_BETA
#define DX_UNIVERSE_TEXT DX_UNIVERSE_BETA_TEXT
#define DX_UNIVERSE_ORIGIN DX_UNIVERSE_BETA_ORIGIN
#endif

// Complain to compiler if DX_UNIVERSE is set higher
#if DX_UNIVERSE > DX_UNIVERSE_SET_PUBLIC
#pragma message("DX_UNIVERSE: This build is not intended for public release.")
#endif

class Version
{
public:
	Version(const int major, const int minor, const int patch) : m_major(major), m_minor(minor), m_patch(patch)
	{
		char* version = new char[16];
		sprintf(version, "%d.%d.%d", m_major, m_minor, m_patch);
		m_version = version;
	}
	~Version()
	{
		delete[] m_version;
	}
	const char* GetVersion() const
	{
		return m_version;
	}
	int m_major;
	int m_minor;
	int m_patch;
protected:
	char* m_version;
};

class CDirectorsCutSystem : public CAutoGameSystemPerFrame
{
public:

	CDirectorsCutSystem() : CAutoGameSystemPerFrame("CDirectorsCutSystem")
	{
	}

	virtual bool InitAllSystems()
	{
		return true;
	}
	
	virtual void PostInit();
	virtual void Shutdown();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	virtual void Update(float frametime);
	void SetupEngineView(Vector &origin, QAngle &angles, float &fov);
	void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16);
	void Perspective(float fov, float aspect, float znear, float zfar, float* m16);
	void OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, float* m16);
	void LookAt(const float* eye, const float* at, const float* up, float* m16);
	void SetDefaultSettings();
	float cameraView[16];
	float cameraProjection[16];
	float identityMatrix[16];
	float snap[3];
	float distance = 100.f;
	float fov = 93; // dunno why this works
	float fovDefault = 93;
	float fovAdjustment = 2;
	float playerFov;
	float camYAngle = 165.f / 180.f * M_PI_F;
	float camXAngle = 32.f / 180.f * M_PI_F;
	float gridSize = 500.f;
	float currentTimeScale = 1.f;
	float timeScale = 1.f;
	Vector pivot;
	Vector newPivot;
	Vector engineOrigin;
	Vector playerOrigin;
	Vector deltaOrigin;
	Vector poseBoneOrigin;
	QAngle engineAngles;
	QAngle playerAngles;
	QAngle deltaAngles;
	QAngle poseBoneAngles;
	CUtlVector < CElementPointer* > elements;
	Version directorcut_version = Version(DX_VERSION_MAJOR, DX_VERSION_MINOR, DX_VERSION_PATCH);
	InputContextHandle_t inputContext;
	CUtlVector<TutorialSection*> tutorialSections;
	char* directorscut_author = "KiwifruitDev";
	char modelName[CHAR_MAX];
	char lightTexture[CHAR_MAX];
	char savePath[MAX_PATH];
	int elementIndex = -1;
	int nextElementIndex = -1;
	int boneIndex = -1;
	int poseIndex = -1;
	int nextPoseIndex = -1;
	int flexIndex = -1;
	int operation = 2;
	int oldOperation = 2;
	int hoveringInfo[3];
	int tutorialSectionIndex = 0;
	bool useSnap = false;
	bool orthographic = false;
	bool firstEndScene = true;
	bool cursorState = false;
	bool imguiActive = false;
	bool levelInit = false;
	bool mainMenu = true;
	bool selecting = false;
	bool justSetPivot = false;
	bool pivotMode = false;
	bool spawnAtPivot = false;
	bool windowVisibilities[4];
	bool inspectorDocked = true;
	bool gotInput = false;
	bool savedOnce = false;
	ImFont* fontTahoma;
};

// singleton
CDirectorsCutSystem &DirectorsCutGameSystem();

// TODO: move this somewhere sensible

// TF2 proxy material dummies
// eventually support will be added to modify these proxies in-editor
// but as it stands, these classes just output "normal" values to solve texture issues

class CDummyProxy : public IMaterialProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	virtual void OnBind(void*) {};
	virtual void Release() {};
	virtual IMaterial* GetMaterial();
	IMaterial* mat;
};

class CDummyProxyResultFloat : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyProxyResultFloatInverted : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyProxyResultRGB : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyProxyResultRGBInverted : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyInvisProxy : public CDummyProxy {};
class CDummySpyInvisProxy : public CDummyProxy {};
class CDummyWeaponInvisProxy : public CDummyProxy {};
class CDummyVmInvisProxy : public CDummyProxy {};
class CDummyBuildingInvisProxy : public CDummyProxy {};
class CDummyCommunityWeaponProxy : public CDummyProxy {};
class CDummyInvulnLevelProxy : public CDummyProxy {};
class CDummyBurnLevelProxy : public CDummyProxyResultFloat {};
class CDummyYellowLevelProxy : public CDummyProxyResultFloatInverted {};
class CDummyModelGlowColorProxy : public CDummyProxyResultRGB {};
class CDummyItemTintColorProxy : public CDummyProxyResultRGB {};
class CDummyBuildingRescueLevelProxy : public CDummyProxy {};
class CDummyTeamTextureProxy : public CDummyProxy {};
class CDummyAnimatedWeaponSheenProxy : public CDummyProxy {};
class CDummyWeaponSkinProxy : public CDummyProxy {};
class CDummyShieldFalloffProxy : public CDummyProxy {};
class CDummyStatTrakIllumProxy : public CDummyProxy {};
class CDummyStatTrakDigitProxy : public CDummyProxy {};
class CDummyStatTrakIconProxy : public CDummyProxy {};
class CDummyStickybombGlowColorProxy : public CDummyProxy {};
class CDummySniperRifleChargeProxy : public CDummyProxy {};
class CDummyHeartbeatProxy : public CDummyProxy {};
class CDummyWheatlyEyeGlowProxy : public CDummyProxy {};
class CDummyBenefactorLevelProxy : public CDummyProxy {};

EXPOSE_INTERFACE(CDummyInvisProxy, IMaterialProxy, "invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummySpyInvisProxy, IMaterialProxy, "spy_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyWeaponInvisProxy, IMaterialProxy, "weapon_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyVmInvisProxy, IMaterialProxy, "vm_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBuildingInvisProxy, IMaterialProxy, "building_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyCommunityWeaponProxy, IMaterialProxy, "CommunityWeapon" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyInvulnLevelProxy, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBurnLevelProxy, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyYellowLevelProxy, IMaterialProxy, "YellowLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyModelGlowColorProxy, IMaterialProxy, "ModelGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyItemTintColorProxy, IMaterialProxy, "ItemTintColor" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBuildingRescueLevelProxy, IMaterialProxy, "BuildingRescueLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyTeamTextureProxy, IMaterialProxy, "TeamTexture" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyAnimatedWeaponSheenProxy, IMaterialProxy, "AnimatedWeaponSheen" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyWeaponSkinProxy, IMaterialProxy, "WeaponSkin" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyShieldFalloffProxy, IMaterialProxy, "ShieldFalloff" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStatTrakIllumProxy, IMaterialProxy, "StatTrakIllum" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStatTrakDigitProxy, IMaterialProxy, "StatTrakDigit" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStatTrakIconProxy, IMaterialProxy, "StatTrakIcon" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStickybombGlowColorProxy, IMaterialProxy, "StickybombGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummySniperRifleChargeProxy, IMaterialProxy, "SniperRifleCharge" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyHeartbeatProxy, IMaterialProxy, "Heartbeat" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyWheatlyEyeGlowProxy, IMaterialProxy, "WheatlyEyeGlow" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBenefactorLevelProxy, IMaterialProxy, "BenefactorLevel" IMATERIAL_PROXY_INTERFACE_VERSION);

#endif
