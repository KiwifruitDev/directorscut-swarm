
#include "cbase.h"
#include "tier0/icommandline.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialvar.h"
#include "filesystem.h"
#include "deferred/deferred_shared_common.h"

#include "vgui_controls/messagebox.h"

static CDeferredManagerClient __g_defmanager;
CDeferredManagerClient *GetDeferredManager()
{
	return &__g_defmanager;
}

static IViewRender *g_pCurrentViewRender = NULL;

IViewRender *GetViewRenderInstance()
{
	AssertMsg( g_pCurrentViewRender != NULL, "viewrender creation failed!" );

	return g_pCurrentViewRender;
}

static CDeferredMaterialSystem g_DeferredMaterialSystem;
static IMaterialSystem *g_pOldMatSystem;


CDeferredManagerClient::CDeferredManagerClient() : BaseClass( "DeferredManagerClient" )
{
	m_bDefRenderingEnabled = false;

	Q_memset( m_pMat_Def, 0, sizeof(IMaterial*) * DEF_MAT_COUNT );
	Q_memset( m_pKV_Def, 0, sizeof(KeyValues*) * DEF_MAT_COUNT );
}

CDeferredManagerClient::~CDeferredManagerClient()
{
}

void CopyDev()
{
	FileFindHandle_t handle;
	char steamappsPath[MAX_PATH*4];
	const char *pszGameDir = engine->GetGameDirectory();

	Q_strcpy( steamappsPath, pszGameDir );
	Q_StripLastDir( steamappsPath, sizeof(steamappsPath) );
	Q_StripLastDir( steamappsPath, sizeof(steamappsPath) );

	char searchPath[MAX_PATH*4];
	Q_snprintf( searchPath, sizeof(searchPath), "%s\\shaders\\fxc\\*", pszGameDir );
	Q_FixSlashes( searchPath );
	Msg( "searching for shaders in: %s\n", searchPath );

	const char *pszName = g_pFullFileSystem->FindFirst( searchPath, &handle );

	while ( pszName != NULL )
	{
		if ( Q_strlen( pszName ) > 4 )
		{
			char filename[MAX_PATH];
			Q_FileBase( pszName, filename, sizeof( filename ) );

			char filepath_src[MAX_PATH];
			char filepath_dst[MAX_PATH];
			Q_snprintf( filepath_src, sizeof( filepath_src ), "%s\\shaders\\fxc\\%s.vcs\0", pszGameDir, filename );
			Q_snprintf( filepath_dst, sizeof( filepath_dst ), "%s\\common\\alien swarm\\platform\\shaders\\fxc\\%s.vcs\0", steamappsPath, filename );
			Q_FixSlashes( filepath_src );
			Q_FixSlashes( filepath_dst );

			Msg( "%s --> %s\n", filepath_src, filepath_dst );
			engine->CopyFile( filepath_src, filepath_dst );
		}

		pszName = g_pFullFileSystem->FindNext( handle );
	}

	g_pFullFileSystem->FindClose( handle );
}

bool CDeferredManagerClient::Init()
{
	CopyDev();

	AssertMsg( g_pCurrentViewRender == NULL, "viewrender already allocated?!" );

	const bool bForceDeferred = CommandLine() && CommandLine()->FindParm("-forcedeferred") != 0;
	bool bSM30 = g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 95;

	if ( !bSM30 )
	{
		Warning( "The engine doesn't recognize your GPU to support SM3.0, running deferred anyway...\n" );
		bSM30 = true;
	}

	if ( bSM30 || bForceDeferred )
	{
		bool bGotDefShaderDll = ConnectDeferredExt();

		if ( bGotDefShaderDll )
		{
			g_pOldMatSystem = materials;

			g_DeferredMaterialSystem.InitPassThru( materials );
			materials = &g_DeferredMaterialSystem;
			engine->Mat_Stub( &g_DeferredMaterialSystem );

			m_bDefRenderingEnabled = true;
			GetDeferredExt()->EnableDeferredLighting();

			g_pCurrentViewRender = new CDeferredViewRender();

			ConVarRef r_shadows( "r_shadows" );
			r_shadows.SetValue( "0" );

			InitDeferredRTs( true );

			materials->AddModeChangeCallBack( &DefRTsOnModeChanged );

			InitializeDeferredMaterials();
		}
	}

	if ( !m_bDefRenderingEnabled )
	{
		Assert( g_pCurrentViewRender == NULL );

		Warning( "Your hardware does not seem to support shader model 3.0. If you think that this is an error (hybrid GPUs), add -forcedeferred as start parameter.\n" );
		g_pCurrentViewRender = new CViewRender();
	}
	else
	{
#define VENDOR_NVIDIA 0x10DE
#define VENDOR_INTEL 0x8086
#define VENDOR_ATI 0x1002
#define VENDOR_AMD 0x1022

#ifndef SHADOWMAPPING_USE_COLOR
		MaterialAdapterInfo_t info;
		materials->GetDisplayAdapterInfo( materials->GetCurrentAdapter(), info );

		if ( info.m_VendorID == VENDOR_ATI ||
			info.m_VendorID == VENDOR_AMD )
		{
			vgui::MessageBox *pATIWarning = new vgui::MessageBox("UNSUPPORTED HARDWARE", VarArgs( "AMD/ATI IS NOT YET SUPPORTED IN HARDWARE FILTERING MODE\n"
				"(cdeferred_manager_client.cpp #%i).", __LINE__ ) );

			pATIWarning->InvalidateLayout();
			pATIWarning->DoModal();
		}
#endif
	}

	return true;
}

void CDeferredManagerClient::Shutdown()
{
	def_light_t::ShutdownSharedMeshes();

	ShutdownDeferredMaterials();
	ShutdownDeferredExt();

	if ( IsDeferredRenderingEnabled() )
	{
		materials->RemoveModeChangeCallBack( &DefRTsOnModeChanged );

		materials = g_pOldMatSystem;
		engine->Mat_Stub( g_pOldMatSystem );
	}

	delete g_pCurrentViewRender;
	g_pCurrentViewRender = NULL;
	view = NULL;
}

ImageFormat CDeferredManagerClient::GetShadowDepthFormat()
{
	ImageFormat f = g_pMaterialSystemHardwareConfig->GetShadowDepthTextureFormat();

	// hack for hybrid stuff
	if ( f == IMAGE_FORMAT_UNKNOWN )
		f = IMAGE_FORMAT_D16_SHADOW;

	return f;
}

ImageFormat CDeferredManagerClient::GetNullFormat()
{
	return g_pMaterialSystemHardwareConfig->GetNullTextureFormat();
}

void CDeferredManagerClient::InitializeDeferredMaterials()
{
#if DEBUG
	m_pKV_Def[ DEF_MAT_WIREFRAME_DEBUG ] = new KeyValues( "wireframe" );
	if ( m_pKV_Def[ DEF_MAT_WIREFRAME_DEBUG ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_WIREFRAME_DEBUG ]->SetString( "$color", "[1 0.5 0.1]" );
		m_pMat_Def[ DEF_MAT_WIREFRAME_DEBUG ] = materials->CreateMaterial( "__lightworld_wireframe", m_pKV_Def[ DEF_MAT_WIREFRAME_DEBUG ] );
	}
#endif

	m_pKV_Def[ DEF_MAT_LIGHT_GLOBAL ] = new KeyValues( "LIGHTING_GLOBAL" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_GLOBAL ] != NULL )
		m_pMat_Def[ DEF_MAT_LIGHT_GLOBAL ] = materials->CreateMaterial( "__lightpass_global", m_pKV_Def[ DEF_MAT_LIGHT_GLOBAL ] );

	m_pKV_Def[ DEF_MAT_LIGHT_POINT_FULLSCREEN ] = new KeyValues( "LIGHTING_WORLD" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_POINT_FULLSCREEN ] != NULL )
		m_pMat_Def[ DEF_MAT_LIGHT_POINT_FULLSCREEN ] = materials->CreateMaterial( "__lightpass_point_fs", m_pKV_Def[ DEF_MAT_LIGHT_POINT_FULLSCREEN ] );

	m_pKV_Def[ DEF_MAT_LIGHT_POINT_WORLD ] = new KeyValues( "LIGHTING_WORLD" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_POINT_WORLD ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_POINT_WORLD ]->SetInt( "$WORLDPROJECTION", 1 );
		m_pMat_Def[ DEF_MAT_LIGHT_POINT_WORLD ] = materials->CreateMaterial( "__lightpass_point_w", m_pKV_Def[ DEF_MAT_LIGHT_POINT_WORLD ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_SPOT_FULLSCREEN ] = new KeyValues( "LIGHTING_WORLD" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_SPOT_FULLSCREEN ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_SPOT_FULLSCREEN ]->SetInt( "$LIGHTTYPE", DEFLIGHTTYPE_SPOT );
		m_pMat_Def[ DEF_MAT_LIGHT_SPOT_FULLSCREEN ] = materials->CreateMaterial( "__lightpass_spot_fs", m_pKV_Def[ DEF_MAT_LIGHT_SPOT_FULLSCREEN ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_SPOT_WORLD ] = new KeyValues( "LIGHTING_WORLD" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_SPOT_WORLD ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_SPOT_WORLD ]->SetInt( "$LIGHTTYPE", DEFLIGHTTYPE_SPOT );
		m_pKV_Def[ DEF_MAT_LIGHT_SPOT_WORLD ]->SetInt( "$WORLDPROJECTION", 1 );
		m_pMat_Def[ DEF_MAT_LIGHT_SPOT_WORLD ] = materials->CreateMaterial( "__lightpass_spot_w", m_pKV_Def[ DEF_MAT_LIGHT_SPOT_WORLD ] );
	}


	/*

	lighting volumes

	*/

	m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_POINT_FULLSCREEN ] = new KeyValues( "LIGHTING_VOLUME" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_POINT_FULLSCREEN ] != NULL )
		m_pMat_Def[ DEF_MAT_LIGHT_VOLUME_POINT_FULLSCREEN ] = materials->CreateMaterial( "__lightpass_point_vfs", m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_POINT_FULLSCREEN ] );

	m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_POINT_WORLD ] = new KeyValues( "LIGHTING_VOLUME" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_POINT_WORLD ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_POINT_WORLD ]->SetInt( "$WORLDPROJECTION", 1 );
		m_pMat_Def[ DEF_MAT_LIGHT_VOLUME_POINT_WORLD ] = materials->CreateMaterial( "__lightpass_point_v", m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_POINT_WORLD ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_FULLSCREEN ] = new KeyValues( "LIGHTING_VOLUME" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_FULLSCREEN ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_FULLSCREEN ]->SetInt( "$LIGHTTYPE", DEFLIGHTTYPE_SPOT );
		m_pMat_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_FULLSCREEN ] = materials->CreateMaterial( "__lightpass_spot_v", m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_FULLSCREEN ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_WORLD ] = new KeyValues( "LIGHTING_VOLUME" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_WORLD ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_WORLD ]->SetInt( "$WORLDPROJECTION", 1 );
		m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_WORLD ]->SetInt( "$LIGHTTYPE", DEFLIGHTTYPE_SPOT );
		m_pMat_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_WORLD ] = materials->CreateMaterial( "__lightpass_spot_v", m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_SPOT_WORLD ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_PREPASS ] = new KeyValues( "VOLUME_PREPASS" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_PREPASS ] != NULL )
		m_pMat_Def[ DEF_MAT_LIGHT_VOLUME_PREPASS ] = materials->CreateMaterial( "__volume_prepass", m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_PREPASS ] );

	m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_BLEND ] = new KeyValues( "VOLUME_BLEND" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_BLEND ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_BLEND ]->SetString( "$BASETEXTURE", GetDefRT_VolumetricsBuffer( 0 )->GetName() );
		m_pMat_Def[ DEF_MAT_LIGHT_VOLUME_BLEND ] = materials->CreateMaterial( "__volume_blend", m_pKV_Def[ DEF_MAT_LIGHT_VOLUME_BLEND ] );
	}

	/*

	radiosity

	*/

	m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_GLOBAL ] = new KeyValues( "RADIOSITY_GLOBAL" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_GLOBAL ] != NULL )
		m_pMat_Def[ DEF_MAT_LIGHT_RADIOSITY_GLOBAL ] = materials->CreateMaterial( "__radpass_global", m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_GLOBAL ] );

	m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_DEBUG ] = new KeyValues( "DEBUG_RADIOSITY_GRID" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_DEBUG ] != NULL )
		m_pMat_Def[ DEF_MAT_LIGHT_RADIOSITY_DEBUG ] = materials->CreateMaterial( "__radpass_dbg_grid", m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_DEBUG ] );

	m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_0 ] = new KeyValues( "RADIOSITY_PROPAGATE" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_0 ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_0 ]->SetString( "$BASETEXTURE", GetDefRT_RadiosityBuffer( 0 )->GetName() );
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_0 ]->SetString( "$NORMALMAP", GetDefRT_RadiosityNormal( 0 )->GetName() );
		m_pMat_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_0 ] = materials->CreateMaterial( "__radpass_prop_0", m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_0 ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_1 ] = new KeyValues( "RADIOSITY_PROPAGATE" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_1 ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_1 ]->SetString( "$BASETEXTURE", GetDefRT_RadiosityBuffer( 1 )->GetName() );
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_1 ]->SetString( "$NORMALMAP", GetDefRT_RadiosityNormal( 1 )->GetName() );
		m_pMat_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_1 ] = materials->CreateMaterial( "__radpass_prop_1", m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_1 ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_0 ] = new KeyValues( "RADIOSITY_PROPAGATE" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_PROPAGATE_0 ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_0 ]->SetInt( "$BLUR", 1 );
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_0 ]->SetString( "$BASETEXTURE", GetDefRT_RadiosityBuffer( 0 )->GetName() );
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_0 ]->SetString( "$NORMALMAP", GetDefRT_RadiosityNormal( 0 )->GetName() );
		m_pMat_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_0 ] = materials->CreateMaterial( "__radpass_blur_0", m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_0 ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_1 ] = new KeyValues( "RADIOSITY_PROPAGATE" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_1 ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_1 ]->SetInt( "$BLUR", 1 );
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_1 ]->SetString( "$BASETEXTURE", GetDefRT_RadiosityBuffer( 1 )->GetName() );
		m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_1 ]->SetString( "$NORMALMAP", GetDefRT_RadiosityNormal( 1 )->GetName() );
		m_pMat_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_1 ] = materials->CreateMaterial( "__radpass_blur_1", m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLUR_1 ] );
	}

	m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLEND ] = new KeyValues( "RADIOSITY_BLEND" );
	if ( m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLEND ] != NULL )
		m_pMat_Def[ DEF_MAT_LIGHT_RADIOSITY_BLEND ] = materials->CreateMaterial( "__radpass_blend", m_pKV_Def[ DEF_MAT_LIGHT_RADIOSITY_BLEND ] );

#if DEFCFG_DEFERRED_SHADING == 1
	/*

	deferred shading

	*/

	m_pKV_Def[ DEF_MAT_SCREENSPACE_SHADING ] = new KeyValues( "SCREENSPACE_SHADING" );
	if ( m_pKV_Def[ DEF_MAT_SCREENSPACE_SHADING ] != NULL )
		m_pMat_Def[ DEF_MAT_SCREENSPACE_SHADING ] = materials->CreateMaterial( "__screenspace_shading", m_pKV_Def[ DEF_MAT_SCREENSPACE_SHADING ] );

	m_pKV_Def[ DEF_MAT_SCREENSPACE_COMBINE ] = new KeyValues( "SCREENSPACE_COMBINE" );
	if ( m_pKV_Def[ DEF_MAT_SCREENSPACE_COMBINE ] != NULL )
		m_pMat_Def[ DEF_MAT_SCREENSPACE_COMBINE ] = materials->CreateMaterial( "__screenspace_combine", m_pKV_Def[ DEF_MAT_SCREENSPACE_COMBINE ] );
#endif

	/*

	blur

	*/

	m_pKV_Def[ DEF_MAT_BLUR_G6_X ] = new KeyValues( "GAUSSIAN_BLUR_6" );
	if ( m_pKV_Def[ DEF_MAT_BLUR_G6_X ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_BLUR_G6_X ]->SetString( "$BASETEXTURE", GetDefRT_VolumetricsBuffer( 0 )->GetName() );
		m_pMat_Def[ DEF_MAT_BLUR_G6_X ] = materials->CreateMaterial( "__blurpass_vbuf_x", m_pKV_Def[ DEF_MAT_BLUR_G6_X ] );
	}

	m_pKV_Def[ DEF_MAT_BLUR_G6_Y ] = new KeyValues( "GAUSSIAN_BLUR_6" );
	if ( m_pKV_Def[ DEF_MAT_BLUR_G6_Y ] != NULL )
	{
		m_pKV_Def[ DEF_MAT_BLUR_G6_Y ]->SetString( "$BASETEXTURE", GetDefRT_VolumetricsBuffer( 1 )->GetName() );
		m_pKV_Def[ DEF_MAT_BLUR_G6_Y ]->SetInt( "$ISVERTICAL", 1 );
		m_pMat_Def[ DEF_MAT_BLUR_G6_Y ] = materials->CreateMaterial( "__blurpass_vbuf_y", m_pKV_Def[ DEF_MAT_BLUR_G6_Y ] );
	}

#if DEBUG
	for ( int i = 0; i < DEF_MAT_COUNT; i++ )
	{
		Assert( m_pKV_Def[ i ] != NULL );
		Assert( m_pMat_Def[ i ] != NULL );
	}
#endif
}

void CDeferredManagerClient::ShutdownDeferredMaterials()
{
	// not deleted on purpose!!!!!
	for ( int i = 0; i < DEF_MAT_COUNT; i++ )
	{
		if ( m_pKV_Def[ i ] != NULL )
			m_pKV_Def[ i ]->Clear();
		m_pKV_Def[ i ] = NULL;
	}
}