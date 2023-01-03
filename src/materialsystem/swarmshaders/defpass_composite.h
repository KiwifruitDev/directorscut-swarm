#ifndef DEFPASS_COMPOSITE_H
#define DEFPASS_COMPOSITE_H

class CDeferredPerMaterialContextData;

struct defParms_composite
{
	defParms_composite()
	{
		Q_memset( this, 0xFF, sizeof( defParms_composite ) );

		bModel = false;
	};

	// textures
	int iAlbedo;
	int iAlbedo2;
	int iAlbedo3;
	int iAlbedo4;
	int iEnvmap;
	int iEnvmapMask;
	int iEnvmapMask2;
	int iBlendmodulate;
	int iBlendmodulate2;
	int iBlendmodulate3;

	// envmapping
	int iEnvmapTint;
	int iEnvmapSaturation;
	int iEnvmapContrast;
	int iEnvmapFresnel;

	// rimlight
	int iRimlightEnable;
	int iRimlightExponent;
	int iRimlightAlbedoScale;
	int iRimlightTint;
	int iRimlightModLight;

	// alpha
	int iAlphatestRef;

	// phong
	int iPhongScale;
	int iPhongFresnel;

	// self illum
	int iSelfIllumTint;
	int iSelfIllumMaskInEnvmapAlpha;
	int iSelfIllumFresnelModulate;
	int iSelfIllumMask;

	// blendmod
	int iBlendmodulateTransform;
	int iBlendmodulateTransform2;
	int iBlendmodulateTransform3;
	int iMultiblend;

	int iFresnelRanges;

	// config
	bool bModel;
};


void InitParmsComposite( const defParms_composite &info, CBaseVSShader *pShader, IMaterialVar **params );
void InitPassComposite( const defParms_composite &info, CBaseVSShader *pShader, IMaterialVar **params );
void DrawPassComposite( const defParms_composite &info, CBaseVSShader *pShader, IMaterialVar **params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData *pDeferredContext );


#endif