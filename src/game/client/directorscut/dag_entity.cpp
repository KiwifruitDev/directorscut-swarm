//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "gamestringpool.h"
#include "datacache/imdlcache.h"
#include "networkstringtable_clientdll.h"
#include "dag_entity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CElementPointer::CElementPointer()
{
	strcpy(name, "\0");
	C_BaseEntity* pEntity = new C_BaseEntity();
	if (!pEntity)
	{
		Msg("Director's Cut: Failed to create generic dag\n");
		return;
	}
	SetPointer(pEntity);
}

CElementPointer::CElementPointer(DAG_ type, KeyValues* params)
{
	strcpy(name, "\0");
	SetType(type);
	Vector pivot = Vector(params->GetFloat("pivotX"), params->GetFloat("pivotY"), params->GetFloat("pivotZ"));
	
	switch (type)
	{
	case DAG_MODEL:
	{
		char* modelName = (char*)params->GetString("modelName");

		// Prepend "models/" if not present
		if (strncmp(modelName, "models/", 7) != 0)
		{
			char newModelName[256];
			sprintf(newModelName, "models/%s", modelName);
			modelName = newModelName;
		}
		// Append ".mdl" if not present
		if (strstr(modelName, ".mdl") == NULL)
		{
			char newModelName[256];
			sprintf(newModelName, "%s.mdl", modelName);
			modelName = newModelName;
		}

		// Cache model
		model_t* model = (model_t*)engine->LoadModel(modelName);
		if (!model)
		{
			Msg("Director's Cut: Failed to load model %s\n", modelName);
			break;
		}
		INetworkStringTable* precacheTable = networkstringtable->FindTable("modelprecache");
		if (precacheTable)
		{
			modelinfo->FindOrLoadModel(modelName);
			int idx = precacheTable->AddString(false, modelName);
			if (idx == INVALID_STRING_INDEX)
			{
				Msg("Director's Cut: Failed to precache model %s\n", modelName);
			}
		}

		// Create test dag
		CModelElement* pEntity = new CModelElement();
		if (!pEntity)
			break;

		pEntity->SetModel(modelName);
		pEntity->SetModelName(modelName);
		pEntity->SetAbsOrigin(pivot);

		KeyValues* childAngles = params->FindKey("angles", true);
		if (childAngles != nullptr)
		{
			pEntity->SetAbsAngles(QAngle(childAngles->GetFloat("x"), childAngles->GetFloat("y"), childAngles->GetFloat("z")));
		}

		pEntity->Spawn();

		// TODO: check to make sure these are in range

		KeyValues* child = params->FindKey("posadds", true);
		for (KeyValues* data = child->GetFirstSubKey(); data != NULL; data = data->GetNextKey())
		{
			Vector posAdd = Vector(data->GetFloat("x"), data->GetFloat("y"), data->GetFloat("z"));
			pEntity->posadds[atoi(data->GetName())] = posAdd;
		}

		KeyValues* anglehelpers = params->FindKey("anglehelper", true);
		for (KeyValues* data = anglehelpers->GetFirstSubKey(); data != NULL; data = data->GetNextKey())
		{
			QAngle anglehelper = QAngle(data->GetFloat("x"), data->GetFloat("y"), data->GetFloat("z"));
			pEntity->anglehelper[atoi(data->GetName())] = anglehelper;
			Msg("anglehelper %i: %f %f %f\n", atoi(data->GetName()), anglehelper.x, anglehelper.y, anglehelper.z);
		}
		// Spawn entity
		RenderGroup_t renderGroup = RENDER_GROUP_OPAQUE;
		if (!pEntity->InitializeAsClientEntity(modelName, renderGroup))
		{
			Msg("Director's Cut: Failed to spawn entity %s\n", modelName);
			pEntity->Release();
			break;
		}

		SetPointer(pEntity);
		break;
	}
	case DAG_LIGHT:
	{
		char* texture = (char*)params->GetString("lightTexture");
		CLightElement* pEntity = new CLightElement();
		if (!pEntity)
		{
			Msg("Director's Cut: Failed to create light with texture %s\n", texture);
			break;
		}
		pEntity->SetAbsOrigin(pivot);
		SetPointer(pEntity);
		break;
	}
	case DAG_CAMERA:
	{
		CCameraElement* pEntity = new CCameraElement();
		if (!pEntity)
		{
			Msg("Director's Cut: Failed to create camera\n");
			break;
		}
		pEntity->SetAbsOrigin(pivot);
		SetPointer(pEntity);
		break;
	}
	default:
	{
		C_BaseEntity* pEntity = new C_BaseEntity();
		if (!pEntity)
		{
			Msg("Director's Cut: Failed to create generic dag\n");
			break;
		}
		pEntity->SetAbsOrigin(pivot);
		SetPointer(pEntity);
		break;
	}
	}
}

CElementPointer::~CElementPointer()
{
	C_BaseEntity* pEntity = (C_BaseEntity*)pElement;
	if (pEntity != nullptr)
	{
		pEntity->Remove();
	}
	C_BaseEntity* pEntityParent = (C_BaseEntity*)pParent;
	if (pEntityParent != nullptr)
	{
		pEntityParent->Remove();
	}
}

void CElementPointer::SetType(DAG_ type)
{
	elementType = type;
}

DAG_ CElementPointer::GetType()
{
	return elementType;
}

// used to avoid setting parent entity pointer
void CElementPointer::SetPointer(void* pElement, bool setParent)
{
	if (setParent && this->pElement != nullptr)
		pParent = this->pElement;
	this->pElement = pElement;
}

// defaults to setting the previous pointer to a "parent" entity
// that entity pointer will be removed when the element is
void CElementPointer::SetPointer(void* pElement)
{
	SetPointer(pElement, true);
}

void* CElementPointer::GetPointer()
{
	return pElement;
}

CModelElement::CModelElement()
{
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
}

CModelElement::~CModelElement()
{
}

void CModelElement::GetRenderBounds(Vector& theMins, Vector& theMaxs)
{
	VectorCopy(Vector(-16384, -16384, -16384), theMins);
	VectorCopy(Vector(16384, 16384, 16384), theMaxs);
}

float CModelElement::GetFlexWeight(LocalFlexController_t index)
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		return forcedFlexes[index];
	}
	return 0.0;
}

void CModelElement::SetFlexWeight(LocalFlexController_t index, float value)
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		forcedFlexes[index] = value;
	}
}

bool CModelElement::Simulate()
{
	C_BaseFlex::Simulate();
	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		C_BaseFlex::SetFlexWeight((LocalFlexController_t)i, forcedFlexes[i]);
	}
	return true;
}

bool CModelElement::SetModel(const char* pModelName)
{
	C_BaseFlex::SetModel(pModelName);
	return true;
}

void CModelElement::PoseBones(int bonenumber, Vector pos, QAngle ang)
{
	posadds[bonenumber] = pos;
	anglehelper[bonenumber] = ang;
}
	

bool CModelElement::SetupBones(matrix3x4a_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	for (int i = 0; i < MAXSTUDIOBONES; i++)
	{
		if (firsttimesetup == false)
		{
			posadds[i].x = 0;
			posadds[i].y = 0;
			posadds[i].z = 0;
			anglehelper[i].x = 0;
			anglehelper[i].y = 0;
			anglehelper[i].z = 0;
		}
		AngleQuaternion(anglehelper[i], qadds[i]);
	}
	firsttimesetup = true;
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
	return C_BaseFlex::SetupBones(pBoneToWorldOut, nMaxBones, boneMask, currentTime, true, posadds, qadds);
}

inline matrix3x4a_t& CModelElement::GetBoneForWrite(int iBone)
{
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
	return C_BaseFlex::GetBoneForWrite(iBone);
}

/*

void CModelElement::ApplyBoneMatrixTransform(matrix3x4_t& transform)
{
	m_BoneAccessor.SetWritableBones(BONE_USED_BY_ANYTHING);
	m_BoneAccessor.SetReadableBones(BONE_USED_BY_ANYTHING);
	PushAllowBoneAccess(true, true, "CModelElement::ApplyBoneMatrixTransform");
	CStudioHdr* hdr = GetModelPtr();
	if (hdr != NULL)
	{
		if (initializedBones)
		{
			// If origin/angles are dirty (old), add their differences to the bone matrix
			Vector origin = GetAbsOrigin();
			QAngle angles = GetAbsAngles();
			if (origin != dirtyOrigin || angles != dirtyAngles)
			{
				Vector diffOrigin = origin - dirtyOrigin;
				QAngle diffAngles = angles - dirtyAngles;
				Vector boneOrigin;
				QAngle boneAngles;
				for (int i = 0; i < hdr->numbones(); i++)
				{
					MatrixAngles(boneMatrices[i], boneAngles, boneOrigin);
					boneAngles += diffAngles;
					boneOrigin += diffOrigin;
					AngleMatrix(boneAngles, boneOrigin, boneMatrices[i]);
				}
				dirtyOrigin = origin;
				dirtyAngles = angles;
			}
		}
		else
		{
			dirtyOrigin = GetAbsOrigin();
			dirtyAngles = GetAbsAngles();
		}
		for (int i = 0; i < hdr->numbones(); i++)
		{
			matrix3x4_t& bone = GetBoneForWrite[i];
			mstudiobone_t* bonePtr = hdr->pBone[i];
			char* name = bonePtr->pszName();

			if(!initializedBones)
			{
				// Initialize bone matrices
				MatrixCopy(bone, boneMatrices[i]);
			}

			matrix3x4_t oldBone;
			MatrixCopy(bone, oldBone);
			
			// Write new bone matrices
			MatrixCopy(boneMatrices[i], bone);
			
			// compare name with "ValveBiped.Bip01_Head1"
			bool compare0 = (oldBone[0][0] != bone[0][0] || oldBone[0][1] != bone[0][1] || oldBone[0][2] != bone[0][2] || oldBone[0][3] != bone[0][3]);

			if (Q_strcmp(name, "ValveBiped.Bip01_Head1") != 0 && (compare0))
				Msg("New bone matrix: %f %f %f %f\n", bone[0][0], bone[0][1], bone[0][2], bone[0][3]);
		}
		if (!initializedBones)
			initializedBones = true;
	}
	PopBoneAccess("CModelElement::ApplyBoneMatrixTransform");
	C_BaseAnimating::ApplyBoneMatrixTransform(transform);
}

*/

CModelElement* CModelElement::CreateRagdollCopy()
{	
	CModelElement* copy = new CModelElement();
	if (copy == NULL)
		return NULL;

	const model_t* model = GetModel();
	const char* pModelName = modelinfo->GetModelName(model);

	if (copy->InitializeAsClientEntity(pModelName, RENDER_GROUP_OPAQUE) == false)
	{
		copy->Release();
		return NULL;
	}

	// move my current model instance to the ragdoll's so decals are preserved.
	SnatchModelInstance(copy);

	// We need to take these from the entity
	Vector origin = GetAbsOrigin();
	QAngle angles = GetAbsAngles();

	copy->SetAbsOrigin(origin);
	copy->SetAbsAngles(angles);

	copy->IgniteRagdoll(this);
	copy->TransferDissolveFrom(this);
	copy->InitModelEffects();
	
	AddEffects(EF_NODRAW);

	if (IsEffectActive(EF_NOSHADOW))
	{
		copy->AddEffects(EF_NOSHADOW);
	}

	// Copy posadds/qadds
	for (int i = 0; i < MAXSTUDIOBONES; i++)
	{
		copy->posadds[i].x = posadds[i].x;
		copy->posadds[i].y = posadds[i].y;
		copy->posadds[i].z = posadds[i].z;
		copy->anglehelper[i].x = anglehelper->x;
		copy->anglehelper[i].y = anglehelper->y;
		copy->anglehelper[i].z = anglehelper->z;
	}

	// Copy flexes
	for (int i = 0; i < MAXSTUDIOFLEXCTRL; i++)
	{
		copy->forcedFlexes[i] = forcedFlexes[i];
	}

	copy->SetModelName(AllocPooledString(pModelName));
	copy->SetModelScale(GetModelScale());
	
	// set up bones now
	copy->SetupBones(NULL, -1, -1, gpGlobals->curtime);
	
	// try this again
	copy->SetAbsOrigin(origin);
	copy->SetAbsAngles(angles);

	copy->SetRenderMode(GetRenderMode());
	copy->SetRenderColor(GetRenderColor().r, GetRenderColor().g, GetRenderColor().b);

	copy->m_nBody = m_nBody;
	copy->m_nSkin = GetSkin();
	copy->m_vecForce = m_vecForce;
	copy->m_nForceBone = m_nForceBone;
	copy->SetNextClientThink(CLIENT_THINK_ALWAYS);

#ifdef MAPBASE
	copy->m_iViewHideFlags = m_iViewHideFlags;

	copy->m_fadeMinDist = m_fadeMinDist;
	copy->m_fadeMaxDist = m_fadeMaxDist;
	copy->m_flFadeScale = m_flFadeScale;
#endif

	/*
	CStudioHdr* hdr = GetModelPtr();
	if (hdr != NULL)
	{
		for (int i = 0; i < hdr->numbones(); i++)
		{
			// Copy bone matrices
			MatrixCopy(boneMatrices[i], copy->boneMatrices[i]);
		}
	}
	*/
	
	return copy;
}

CModelElement* CModelElement::BecomeRagdollOnClient()
{
	MoveToLastReceivedPosition(true);
	GetAbsOrigin();
	CModelElement* pRagdoll = CreateRagdollCopy();

	matrix3x4a_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4a_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4a_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.1f;
	GetRagdollInitBoneArrays(boneDelta0, boneDelta1, currentBones, boneDt);
	pRagdoll->InitAsClientRagdoll(boneDelta0, boneDelta1, currentBones, boneDt);

#ifdef MAPBASE_VSCRIPT
	// Hook for ragdolling
	if (m_ScriptScope.IsInitialized() && g_Hook_OnClientRagdoll.CanRunInScope(m_ScriptScope))
	{
		// ragdoll
		ScriptVariant_t args[] = { ScriptVariant_t(pRagdoll->GetScriptInstance()) };
		g_Hook_OnClientRagdoll.Call(m_ScriptScope, NULL, args);
	}
#endif

	return pRagdoll;
}

CLightElement::CLightElement()
{
	TurnOn();
}

CLightElement::~CLightElement()
{
	if (m_FlashlightHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyFlashlight(m_FlashlightHandle);
		m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void CLightElement::UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance)
{
	if (IsOn() == false)
		return;

	FlashlightState_t state;
	Vector basisX, basisY, basisZ;
	basisX = vecDir;
	basisY = vecRight;
	basisZ = vecUp;
	VectorNormalize(basisX);
	VectorNormalize(basisY);
	VectorNormalize(basisZ);

	BasisToQuaternion(basisX, basisY, basisZ, state.m_quatOrientation);

	state.m_vecLightOrigin = vecPos;

	state.m_fHorizontalFOVDegrees = 90.0f;
	state.m_fVerticalFOVDegrees = 90.0f;
	state.m_fQuadraticAtten = 0.0f;
	state.m_fLinearAtten = 100.0f;
	state.m_fConstantAtten = 0.0f;
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = 0.0f;
	state.m_NearZ = 4.0f;
	state.m_FarZ = 750.0f;
	state.m_bEnableShadows = true;
	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_nSpotlightTextureFrame = 0;

	if (GetFlashlightHandle() == CLIENTSHADOW_INVALID_HANDLE)
	{
		Msg("Creating flashlight\n");
		SetFlashlightHandle(g_pClientShadowMgr->CreateFlashlight(state));
	}
	else
	{
		g_pClientShadowMgr->UpdateFlashlightState(GetFlashlightHandle(), state);
	}

	g_pClientShadowMgr->UpdateProjectedTexture(GetFlashlightHandle(), true);
}
