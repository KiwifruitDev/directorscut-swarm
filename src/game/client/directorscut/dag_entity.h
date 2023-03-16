//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DAG_ENTITY_H_
#define _DAG_ENTITY_H_

#include "c_baseentity.h"
#include "c_baseanimating.h"
#include "basetypes.h"
#include "flashlighteffect.h"
#include "c_baseflex.h"
#include "c_point_camera.h"
#include "deferred/deferred_shared_common.h"
#include "deferred/cdeflight.h"

enum DAG_
{
	DAG_NONE = 0,
	DAG_MODEL,
	DAG_LIGHT,
	DAG_CAMERA,
	DAG_MAX
};

// this is a base class for all entities that are used in Director's Cut 
// used for CUtlVector and serialization
class CElementPointer
{
public:
	CElementPointer();
	CElementPointer(DAG_ type, KeyValues* params);
	~CElementPointer();
	void SetType(DAG_ type);
	DAG_ GetType();
	void SetPointer(void* pElement, bool setParent);
	void SetPointer(void* pElement);
	void* GetPointer();
	char name[CHAR_MAX];
protected:
	DAG_ elementType = DAG_NONE;
	void* pElement = NULL;
	void* pParent = NULL; // ragdoll source entity
};

class CModelElement : public C_BaseFlex
{
	DECLARE_CLASS(CModelElement, C_BaseFlex);
public:
	CModelElement();
	~CModelElement();
	virtual bool	Simulate();
	CModelElement* BecomeRagdollOnClient();
	CModelElement* CreateRagdollCopy();
	virtual bool SetModel(const char* pModelName);
	void PoseBones(int bonenumber, Vector pos, QAngle ang);
	virtual matrix3x4a_t& GetBoneForWrite(int iBone);
	virtual bool SetupBones(matrix3x4a_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	virtual float GetFlexWeight(LocalFlexController_t index);
	virtual void SetFlexWeight(LocalFlexController_t index, float value);
	//virtual void ApplyBoneMatrixTransform(matrix3x4_t& transform);
	bool firsttimesetup = false;
	Vector posadds[MAXSTUDIOBONES];
	QAngle anglehelper[MAXSTUDIOBONES];
	QuaternionAligned qadds[MAXSTUDIOBONES];
	float forcedFlexes[MAXSTUDIOFLEXCTRL];
	virtual void GetRenderBounds(Vector& theMins, Vector& theMaxs);
};

class CLightElement : public C_BaseEntity, public CFlashlightEffect
{
	DECLARE_CLASS(CLightElement, C_BaseEntity);
public:
	CLightElement();
	~CLightElement();
	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance);
};

class CCameraElement : public C_PointCamera
{
	DECLARE_CLASS(CCameraElement, C_PointCamera);
public:
	void SetActive(bool active);
};

#endif
