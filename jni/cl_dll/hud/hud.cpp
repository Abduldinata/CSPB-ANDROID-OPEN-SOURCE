/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// hud.cpp
//
// implementation of CHud class
//

#ifdef _WIN32
#include "port.h"
#endif

#include <new>
#include <string>
#include <vector>

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "parsemsg.h"
#include "calcscreen.h"


#include "demo.h"
#include "demo_api.h"
#include "vgui_parser.h"
#include "rain.h"

#include "camera.h"

#include "cs_wpn/bte_weapons.h"
#include "triangleapi.h"
#include "draw_util.h"
 
#include "r_efx.h"
#include "event_api.h"
#include "com_model.h"

extern client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

wrect_t nullrc = { 0, 0, 0, 0 };
float g_lastFOV = 0.0;
const char *sPlayerModelFiles[12] =
{
	"models/player.mdl",
	"models/player/leet/leet.mdl", // t
	"models/player/gign/gign.mdl", // ct
	"models/player/gign/gign.mdl", // vip fallback for Android recovery build
	"models/player/gsg9/gsg9.mdl", // ct
	"models/player/guerilla/guerilla.mdl", // t
	"models/player/arctic/arctic.mdl", // t
	"models/player/sas/sas.mdl", // ct
	"models/player/terror/terror.mdl", // t
	"models/player/urban/urban.mdl", // ct
	"models/player/spetsnaz/spetsnaz.mdl", // ct
	"models/player/militia/militia.mdl" // t
};

#define GHUD_DECLARE_MESSAGE(x) int __MsgFunc_##x(const char *pszName, int iSize, void *pbuf ) { return gHUD.MsgFunc_##x(pszName, iSize, pbuf); }

GHUD_DECLARE_MESSAGE(Logo)
GHUD_DECLARE_MESSAGE(SetFOV)
GHUD_DECLARE_MESSAGE(InitHUD)
GHUD_DECLARE_MESSAGE(Concuss)
GHUD_DECLARE_MESSAGE(ResetHUD)
GHUD_DECLARE_MESSAGE(ViewMode)
GHUD_DECLARE_MESSAGE(GameMode)
GHUD_DECLARE_MESSAGE(ShadowIdx)

void __CmdFunc_InputCommandSpecial()
{
#ifdef _CS16CLIENT_ALLOW_SPECIAL_SCRIPTING
	gEngfuncs.pfnClientCmd("_special");
#endif
}

void __CmdFunc_GunSmoke()
{
	if( gHUD.cl_gunsmoke->value )
		gEngfuncs.Cvar_SetValue( "cl_gunsmoke", 0 );
	else
		gEngfuncs.Cvar_SetValue( "cl_gunsmoke", 1 );
}

static int CSPB_ClampLobbyClassIndex( int classIndex )
{
	if( classIndex < 1 )
		return 1;

	if( classIndex > 5 )
		return 5;

	return classIndex;
}

static void CSPB_RunLobbyClassAlias(const char *prefix, int classIndex)
{
	if (!prefix || !prefix[0])
		return;

	char command[64];
	snprintf(command, sizeof(command), "%s%d\n", prefix, CSPB_ClampLobbyClassIndex(classIndex));
	command[sizeof(command) - 1] = '\0';

	gEngfuncs.pfnClientCmd(command);
}

void __CmdFunc_PB_OpenActiveLobbyMenu()
{
	// 1. Run Map and Mode Aliases
	char command[128];
	const char *levelName = gEngfuncs.pfnGetLevelName();
	if ( levelName && levelName[0] )
	{
		// levelName is usually "maps/pb_luxville.bsp"
		// We want just the basename "pb_luxville"
		const char *slash = strrchr( levelName, '/' );
		if ( !slash ) slash = strrchr( levelName, '\\' );
		const char *base = slash ? slash + 1 : levelName;
		
		char mapBase[64];
		strncpy( mapBase, base, sizeof( mapBase ) - 1 );
		mapBase[sizeof( mapBase ) - 1] = 0;
		
		char *dot = strrchr( mapBase, '.' );
		if ( dot ) *dot = 0;
		
		snprintf( command, sizeof( command ), "_db_map_%s\n", mapBase );
		gEngfuncs.pfnClientCmd( command );
	}

	if (gHUD.pb_active_mode && gHUD.pb_active_mode->string && gHUD.pb_active_mode->string[0])
	{
		snprintf(command, sizeof(command), "_db_mode_%s\n", gHUD.pb_active_mode->string);
		command[sizeof(command) - 1] = '\0';
		gEngfuncs.pfnClientCmd(command);
	}
	// 2. Run Class Alias
	int currentTeam = 2;
	if( gHUD.m_pbteam )
		currentTeam = (int)gHUD.m_pbteam->value;

	if( currentTeam == 1 )
	{
		int redClass = 1;
		if( gHUD.pb_active_red_class )
			redClass = (int)gHUD.pb_active_red_class->value;

		CSPB_RunLobbyClassAlias( "_selected_red_class_", redClass );
	}
	else
	{
		int blueClass = 1;
		if( gHUD.pb_active_blue_class )
			blueClass = (int)gHUD.pb_active_blue_class->value;

		CSPB_RunLobbyClassAlias( "_selected_blue_class_", blueClass );
	}
}

void __CmdFunc_PB_SelectBlueClass()
{
	int classIndex = 1;

	if( gEngfuncs.Cmd_Argc() > 1 )
		classIndex = atoi( gEngfuncs.Cmd_Argv( 1 ) );

	classIndex = CSPB_ClampLobbyClassIndex( classIndex );
	gEngfuncs.Cvar_SetValue( "pb_active_blue_class", classIndex );
	gEngfuncs.Cvar_SetValue( "pbblueselect", classIndex );
	gEngfuncs.Cvar_SetValue( "pbteamselect", 2 );
}

void __CmdFunc_PB_SelectRedClass()
{
	int classIndex = 1;

	if( gEngfuncs.Cmd_Argc() > 1 )
		classIndex = atoi( gEngfuncs.Cmd_Argv( 1 ) );

	classIndex = CSPB_ClampLobbyClassIndex( classIndex );
	gEngfuncs.Cvar_SetValue( "pb_active_red_class", classIndex );
	gEngfuncs.Cvar_SetValue( "pbredselect", classIndex );
	gEngfuncs.Cvar_SetValue( "pbteamselect", 1 );
}

void __CmdFunc_PB_ToggleLobbySide()
{
	int currentTeam = 2;

	if( gHUD.m_pbteam )
		currentTeam = (int)gHUD.m_pbteam->value;

	if( currentTeam == 2 )
	{
		int redClass = 1;
		if( gHUD.pb_active_red_class )
			redClass = (int)gHUD.pb_active_red_class->value;

		CSPB_RunLobbyClassAlias( "_selected_red_class_", redClass );
		return;
	}

	int blueClass = 1;
	if( gHUD.pb_active_blue_class )
		blueClass = (int)gHUD.pb_active_blue_class->value;

	CSPB_RunLobbyClassAlias( "_selected_blue_class_", blueClass );
}

static void CSPB_PreloadLobbyUiSounds( void )
{
	static bool s_preloaded = false;

	if( s_preloaded )
		return;

	static const char *kLobbyUiSounds[] =
	{
		"addons/neda/ui/enter.wav",
		"addons/neda/ui/click.wav",
		"addons/neda/ui/switch.wav",
		"addons/neda/ui/use.wav",
		"addons/neda/ui/back.wav"
	};

	for( size_t i = 0; i < ( sizeof( kLobbyUiSounds ) / sizeof( kLobbyUiSounds[0] )); ++i )
	{
		// Force the client audio path to register/load lobby UI sounds early.
		// This reduces first-use stalls later when CFG touch buttons begin calling `play`.
		gEngfuncs.pfnPlaySoundByName( const_cast<char *>( kLobbyUiSounds[i] ), 0.0f );
	}

	s_preloaded = true;
	gEngfuncs.Con_Printf( "CSPB_ANDROID_BLANKOUT: preloaded lobby UI sounds\n" );
}

#define XASH_GENERATE_BUILDNUM

#if defined(XASH_GENERATE_BUILDNUM)
static const char *date = __DATE__;
static const char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "August", "Sep", "Oct", "Nov", "Dec" };
static char mond[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
#endif

char *Q_buildnum( void )
{
// do not touch this! Only author of Xash3D can increase buildnumbers!
// Xash3D SDL: HAHAHA! I TOUCHED THIS!
	int m = 0, d = 0, y = 0;
	static int b = 0;
	static char buildnum[16];

	if( b != 0 )
		return buildnum;

	for( m = 0; m < 11; m++ )
	{
		if( !strncasecmp( &date[0], mon[m], 3 ))
			break;
		d += mond[m];
	}

	d += atoi( &date[4] ) - 1;
	y = atoi( &date[7] ) - 1900;
	b = d + (int)((y - 1) * 365.25f );

	if((( y % 4 ) == 0 ) && m > 1 )
	{
		b += 1;
	}
	//b -= 38752; // Feb 13 2007
	b -= 41940; // Oct 29 2015.
	// Happy birthday, cs16client! :)

	snprintf( buildnum, sizeof(buildnum), "%i", b );

	return buildnum;
}

int __MsgFunc_ADStop( const char *name, int size, void *buf ) { return 1; }
int __MsgFunc_ItemStatus( const char *name, int size, void *buf ) { return 1; }
int __MsgFunc_ReqState( const char *name, int size, void *buf ) { return 1; }
int __MsgFunc_ForceCam( const char *name, int size, void *buf ) { return 1; }
int __MsgFunc_Spectator( const char *name, int size, void *buf ) { return 1; }
int __MsgFunc_ServerName(const char *name, int size, void *buf)
{
	BufferReader reader(name, buf, size);
	const char *serverName = reader.ReadString();

	if (!serverName)
		serverName = "";

	strncpy(gHUD.m_szServerName, serverName, sizeof(gHUD.m_szServerName) - 1);
	gHUD.m_szServerName[sizeof(gHUD.m_szServerName) - 1] = '\0';

	return 1;
}

#ifdef __ANDROID__
bool evdev_open = false;
void __CmdFunc_MouseSucksOpen( void ) { evdev_open = true; }
void __CmdFunc_MouseSucksClose( void ) { evdev_open = false; }
#endif

// This is called every time the DLL is loaded
void CHud :: Init( void )
{
	gEngfuncs.Con_Printf("CSPB_ANDROID_BLANKOUT: CHud::Init start\n");
	CSPB_PreloadLobbyUiSounds();
	HOOK_COMMAND( "special", InputCommandSpecial );
	HOOK_COMMAND( "PB_SelectBlueClass", PB_SelectBlueClass );
	HOOK_COMMAND( "PB_SelectRedClass", PB_SelectRedClass );
	HOOK_COMMAND( "PB_OpenActiveLobbyMenu", PB_OpenActiveLobbyMenu );
	HOOK_COMMAND( "PB_ToggleLobbySide", PB_ToggleLobbySide );
	//HOOK_COMMAND( "gunsmoke", GunSmoke );

#ifdef __ANDROID__
	HOOK_COMMAND( "evdev_mouseopen", MouseSucksOpen );
	HOOK_COMMAND( "evdev_mouseclose", MouseSucksClose );
#endif
	
	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( Concuss );

	HOOK_MESSAGE( ADStop );
	HOOK_MESSAGE( ItemStatus );
	HOOK_MESSAGE( ReqState );
	HOOK_MESSAGE( ForceCam );
	HOOK_MESSAGE( Spectator ); // ignored due to touch menus
	HOOK_MESSAGE( ServerName );


	HOOK_MESSAGE( ShadowIdx );
	gEngfuncs.Con_Printf("CSPB_ANDROID_BLANKOUT: base HUD messages hooked\n");

	CVAR_CREATE( "_vgui_menus", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	CVAR_CREATE( "_cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	CVAR_CREATE( "_ah", "0", FCVAR_ARCHIVE | FCVAR_USERINFO );

	hud_textmode = CVAR_CREATE( "hud_textmode", "0", FCVAR_ARCHIVE );
	hud_colored  = CVAR_CREATE( "hud_colored", "0", FCVAR_ARCHIVE );
	cl_righthand = CVAR_CREATE( "hand", "1", FCVAR_ARCHIVE );
	cl_weather   = CVAR_CREATE( "cl_weather", "1", FCVAR_ARCHIVE );
	cl_minmodels = CVAR_CREATE( "cl_minmodels", "0", FCVAR_ARCHIVE );
	cl_min_t     = CVAR_CREATE( "cl_min_t", "1", FCVAR_ARCHIVE );
	cl_min_ct    = CVAR_CREATE( "cl_min_ct", "2", FCVAR_ARCHIVE );
	cl_lw        = gEngfuncs.pfnGetCvarPointer( "cl_lw" );
	cl_predict   = gEngfuncs.pfnGetCvarPointer( "cl_predict" );
#ifdef __ANDROID__
	cl_android_force_defaults  = CVAR_CREATE( "cl_android_force_defaults", "1", FCVAR_ARCHIVE );
#endif
	cl_shadows   = CVAR_CREATE( "cl_shadows", "1", FCVAR_ARCHIVE );
	default_fov  = CVAR_CREATE( "default_fov", "90", 0 );
	m_pCvarDraw  = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	fastsprites  = CVAR_CREATE( "fastsprites", "0", FCVAR_ARCHIVE );
m_csgohud  = CVAR_CREATE( "pb_hud", "0", FCVAR_ARCHIVE );

m_pbhudvar  = CVAR_CREATE( "pb_hud_variant", "0", FCVAR_ARCHIVE );

m_hudevo  = CVAR_CREATE( "pb_evo_hud", "0", FCVAR_ARCHIVE );

//annhs
m_annhs  = CVAR_CREATE( "annhs", "0", 0 );

//laser
m_laser  = CVAR_CREATE( "laser", "0", FCVAR_ARCHIVE );

//gui
//m_spectatorgui  = CVAR_CREATE( "spectator_mode", "0", 0 );

m_respawnann  = CVAR_CREATE( "respawnann", "0", 0 );

//pb team
m_pbteam  = CVAR_CREATE( "pbteamselect", "1", FCVAR_ARCHIVE );


//team
m_pbredclass  = CVAR_CREATE( "pbredselect", "1", FCVAR_ARCHIVE );

m_pbblueclass  = CVAR_CREATE( "pbblueselect", "1", FCVAR_ARCHIVE );
pb_active_red_class = CVAR_CREATE( "pb_active_red_class", "1", FCVAR_ARCHIVE );
pb_active_blue_class = CVAR_CREATE( "pb_active_blue_class", "1", FCVAR_ARCHIVE );
pb_active_mode = CVAR_CREATE( "pb_active_mode", "tdm", FCVAR_ARCHIVE );

pb_mission  = CVAR_CREATE( "pb_mission", "1", FCVAR_ARCHIVE );

m_pb_crosshair  = CVAR_CREATE( "pb_crosshair", "0", 0 );

//pos
m_starpos  = CVAR_CREATE( "starpos", "0", 0 ); //0 is pos 1, 1 is pos 2

m_disable_ek  = CVAR_CREATE( "disable_effect_kill", "0", 0 );


helmet_enable = CVAR_CREATE( "helmet_enable", "0", 0 );

//killtest
m_killcmd1  = CVAR_CREATE( "billflxkill1", "0", 0 );
m_killcmd2  = CVAR_CREATE( "billflxkill2", "0", 0 );
m_killcmd3  = CVAR_CREATE( "billflxkill3", "0", 0 );
m_killcmd4  = CVAR_CREATE( "billflxkill4", "0", 0 );
m_headshot  = CVAR_CREATE( "billflxheadshot", "0", 0 );
m_chainheadshot  = CVAR_CREATE( "billflxchainheadshot", "0", 0 );
m_chainslugger  = CVAR_CREATE( "billflxchainslugger", "0", 0 );
m_helmet  = CVAR_CREATE( "billflxhelmet", "0", 0 );

m_iconbomb  = CVAR_CREATE( "bombicon", "0", 0 );

m_hspoint  = CVAR_CREATE( "billflxhspoint", "0", 0 );

m_pointcmd  = CVAR_CREATE( "billflxpoint", "0", 0 );
m_stopper  = CVAR_CREATE( "billflxstopper", "0", 0 );

m_animcmd  = CVAR_CREATE( "billflxanim", "1", 0 );

m_redccmd  = CVAR_CREATE( "billflxredccmd", "0", 0 );

//test star
m_star1cmd  = CVAR_CREATE( "billflxstar1cmd", "0", 0 );
m_star2cmd  = CVAR_CREATE( "billflxstar2cmd", "0", 0 );
m_star3cmd  = CVAR_CREATE( "billflxstar3cmd", "0", 0 );
m_star4cmd  = CVAR_CREATE( "billflxstar4cmd", "0", 0 );
m_star5cmd  = CVAR_CREATE( "billflxstar5cmd", "0", 0 );
m_star6cmd  = CVAR_CREATE( "billflxstar6cmd", "0", 0 );
m_star7cmd  = CVAR_CREATE( "billflxstar7cmd", "0", 0 );
m_star8cmd  = CVAR_CREATE( "billflxstar8cmd", "0", 0 );
m_star9cmd  = CVAR_CREATE( "billflxstar9cmd", "0", 0 );
m_star10cmd  = CVAR_CREATE( "billflxstar10cmd", "0", 0 );

//inv

// Keep selected inventory slots across restarts so lobby equip state survives beyond alias-only CFG flow.
inventory_primary = CVAR_CREATE( "inventory_primary", "20", FCVAR_ARCHIVE );//default k2
inventory_secondary = CVAR_CREATE( "inventory_secondary", "3", FCVAR_ARCHIVE );//default k5
inventory_melee = CVAR_CREATE( "inventory_melee", "3", FCVAR_ARCHIVE );//default  knife 
inventory_explosive = CVAR_CREATE( "inventory_explosive", "1", FCVAR_ARCHIVE );
inventory_special = CVAR_CREATE( "inventory_special", "1", FCVAR_ARCHIVE );
inv_profile = CVAR_CREATE( "inv_profile", "default", FCVAR_ARCHIVE );

//win
m_bluewinann  = CVAR_CREATE( "bluewin", "0", 0 );
m_redwinann  = CVAR_CREATE( "redwin", "0", 0 );


slugger_killed = CVAR_CREATE( "slugger_killed", "0", 0 );

//klclear kill
m_clearkill  = CVAR_CREATE( "clearlistkill", "0", 0 );

	cl_gunsmoke  = CVAR_CREATE( "cl_gunsmoke", "0", FCVAR_ARCHIVE );
	cl_weapon_sparks = CVAR_CREATE( "cl_weapon_sparks", "1", FCVAR_ARCHIVE );
	cl_weapon_wallpuff = CVAR_CREATE( "cl_weapon_wallpuff", "1", FCVAR_ARCHIVE );
	zoom_sens_ratio = CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", 0 );
	sv_skipshield = gEngfuncs.pfnGetCvarPointer( "sv_skipshield" );


custom_scope_cmd = CVAR_CREATE( "custom_scope_cmd", "0", FCVAR_ARCHIVE );

//items 
item_QuickDeploy = CVAR_CREATE( "billflxcrypted_quickdeploy_enable", "0", 1<<21);
item_QuickReload = CVAR_CREATE( "billflxcrypted_quickreload_enable", "0", 1<<21);
item_MegaHp = CVAR_CREATE( "billflxcrypted_megahp_enable", "0", 1<<21);
item_mask = CVAR_CREATE( "billflxcrypted_item_mask", "0", 1<<21);
item_qrespawn = CVAR_CREATE( "billflxcrypted_item_qrespawn", "0", 1<<21);

bought_item_QuickDeploy = CVAR_CREATE( "billflxcrypted_bought_quickdeploy", "0", 1<<21);
bought_item_QuickReload = CVAR_CREATE( "billflxcrypted_bought_quickreload", "0", 1<<21);
bought_item_MegaHp = CVAR_CREATE( "billflxcrypted_bought_megahp", "0", 1<<21);
bought_item_mask_1 = CVAR_CREATE( "billflxcrypted_bought_mask_1", "0", 1<<21);
bought_item_mask_2 = CVAR_CREATE( "billflxcrypted_bought_mask_2", "0", 1<<21);
bought_item_mask_3 = CVAR_CREATE( "billflxcrypted_bought_mask_3", "0", 1<<21);
bought_item_mask_4 = CVAR_CREATE( "billflxcrypted_bought_mask_4", "0", 1<<21);
bought_item_mask_5 = CVAR_CREATE( "billflxcrypted_bought_mask_5", "0", 1<<21);
bought_item_mask_6 = CVAR_CREATE( "billflxcrypted_bought_mask_6", "0", 1<<21);
bought_item_mask_7 = CVAR_CREATE( "billflxcrypted_bought_mask_7", "0", 1<<21);
bought_item_mask_8 = CVAR_CREATE( "billflxcrypted_bought_mask_8", "0", 1<<21);
bought_item_mask_9 = CVAR_CREATE( "billflxcrypted_bought_mask_9", "0", 1<<21);
bought_item_mask_10 = CVAR_CREATE( "billflxcrypted_bought_mask_10", "0", 1<<21);
bought_item_mask_11 = CVAR_CREATE( "billflxcrypted_bought_mask_11", "0", 1<<21);
bought_item_mask_12 = CVAR_CREATE( "billflxcrypted_bought_mask_12", "0", 1<<21);
bought_item_mask_13 = CVAR_CREATE( "billflxcrypted_bought_mask_13", "0", 1<<21);
bought_item_qrespawn = CVAR_CREATE( "billflxcrypted_bought_qrespawn", "0", 1<<21);

unit_item_QuickDeploy = CVAR_CREATE( "billflxcrypted_unit_quickdeploy", "100", 1<<21);
unit_item_QuickReload = CVAR_CREATE( "billflxcrypted_unit_quickreload", "100", 1<<21);
unit_item_MegaHp = CVAR_CREATE( "billflxcrypted_unit_megahp", "100", 1<<21);
unit_item_mask_1 = CVAR_CREATE( "billflxcrypted_unit_mask_1", "100", 1<<21);
unit_item_mask_2 = CVAR_CREATE( "billflxcrypted_unit_mask_2", "100", 1<<21);
unit_item_mask_3 = CVAR_CREATE( "billflxcrypted_unit_mask_3", "100", 1<<21);
unit_item_mask_4 = CVAR_CREATE( "billflxcrypted_unit_mask_4", "100", 1<<21);
unit_item_mask_5 = CVAR_CREATE( "billflxcrypted_unit_mask_5", "100", 1<<21);
unit_item_mask_6 = CVAR_CREATE( "billflxcrypted_unit_mask_6", "100", 1<<21);
unit_item_mask_7 = CVAR_CREATE( "billflxcrypted_unit_mask_7", "100", 1<<21);
unit_item_mask_8 = CVAR_CREATE( "billflxcrypted_unit_mask_8", "100", 1<<21);
unit_item_mask_9 = CVAR_CREATE( "billflxcrypted_unit_mask_9", "100", 1<<21);
unit_item_mask_10 = CVAR_CREATE( "billflxcrypted_unit_mask_10", "100", 1<<21);
unit_item_mask_11 = CVAR_CREATE( "billflxcrypted_unit_mask_11", "100", 1<<21);
unit_item_mask_12 = CVAR_CREATE( "billflxcrypted_unit_mask_12", "100", 1<<21);
unit_item_mask_13 = CVAR_CREATE( "billflxcrypted_unit_mask_13", "100", 1<<21);
unit_item_qrespawn = CVAR_CREATE( "billflxcrypted_unit_qrespawn", "100", 1<<21);

weaponName = CVAR_CREATE( "weaponName", "NULL", 0 );

enable_key_overlay = CVAR_CREATE( "enable_keyboard_overlay", "0", FCVAR_ARCHIVE );
enable_healthbar_overlay = CVAR_CREATE( "enable_healthbar_overlay", "1", FCVAR_ARCHIVE );

pb_new_hud = CVAR_CREATE( "pb_new_hud", "0", FCVAR_ARCHIVE );

hitmark_size = CVAR_CREATE( "hitmark_size", "25", FCVAR_ARCHIVE );

	cl_headname = CVAR_CREATE("cl_headname", "0", FCVAR_ARCHIVE); // seems lagging, disable by default.

	CVAR_CREATE( "cscl_ver", Q_buildnum(), 1<<14 | FCVAR_USERINFO ); // init and userinfo

hideRadarScore = TRUE;
hideRadar = FALSE;

	m_iLogo = 0;
	m_iFOV = 0;

	m_pSpriteList = NULL;

	// Clear any old HUD list
	for( HUDLIST *pList = m_pHudList; pList; pList = m_pHudList )
	{
		m_pHudList = m_pHudList->pNext;
		delete pList;
	}
	m_pHudList = NULL;

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;
	m_iNoConsolePrint = 0;
	m_szServerName[0] = 0;

	Localize_Init();


	// fullscreen overlays
	
	m_NVG.Init();
	m_Retina.Init();
	m_SpectatorGui.Init();

	// Game HUD things
m_SniperScope.Init();
m_Ammo.Init();

m_DeathScreen.Init();
m_DamagePb.Init();
m_Health.Init();
	m_Radio.Init();
	m_Timer.Init();
	m_Money.Init();
	m_AmmoSecondary.Init();
	m_Train.Init();
	m_Battery.Init();
	m_StatusIcons.Init();
	// CSPB Blankout runs only non-zombie modes. Keep zombie HUD modules disabled
	// so they don't hook messages / load HUD assets during normal TDM startup.
	//m_ZBS.Init();
	//m_ZB2.Init();
	//m_ZB3.Init();
	m_MoeTouch.Init();

	// chat, death notice, status bars and other
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_TextMessage.Init();
	m_FollowIcon.Init();
	m_MOTD.Init();
	m_scenarioStatus.Init();
	m_HeadName.Init();
	m_HealthBar.Init();

	m_KillEffect.Init();

	m_Radar.Init();
	m_Scoreboard.Init();

m_InventoryGive.Init();
m_Mission_Announcement_Red.Init();
m_Mission_Announcement_Blue.Init();
m_InventoryUi.Init();

	m_Keys.Init();

	// all things that have own background and must be drawn last
	m_ProgressBar.Init();
	m_Menu.Init();
	

	InitRain();

	BTEClientWeapons().Init();

	//ServersInit();

	gEngfuncs.Cvar_SetValue( "hand", 1 );
	gEngfuncs.Cvar_SetValue( "sv_skipshield", 1.0f );
#if defined(__ANDROID__) || defined(TARGET_OS_IPHONE )
	gEngfuncs.Cvar_SetValue( "hud_fastswitch", 1 );
#endif

	MsgFunc_ResetHUD(0, 0, NULL );
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud :: ~CHud()
{
	delete [] m_rghSprites;
	delete [] m_rgrcRects;
	delete [] m_rgszSpriteNames;

	// Clear any old HUD list
	for( HUDLIST *pList = m_pHudList; pList; pList = m_pHudList )
	{
		m_pHudList = m_pHudList->pNext;
		delete pList;
	}
	m_pHudList = NULL;
}

void CHud :: VidInit( void )
{
	static bool firstinit = true;
	m_scrinfo.iSize = sizeof( m_scrinfo );
	GetScreenInfo( &m_scrinfo );

	m_truescrinfo.iWidth = CVAR_GET_FLOAT("width");
	m_truescrinfo.iHeight = CVAR_GET_FLOAT("height");

	// ----------
	// Load Sprites
	// ---------
	//	m_hsprFont = LoadSprite("sprites/%d_font.spr");
	
	m_hsprLogo = 0;

	// assume cs16-client is launched in landscape mode
	// must be only TrueWidth, but due to bug game may sometime rotate to portait mode
	// calc scale depending on max side
	float maxScale = (float)max( TrueWidth, TrueHeight ) / 640.0f;
	
	// REMOVE LATER
	float currentScale = CVAR_GET_FLOAT("barrettsniperm82a1billflxcryptedcommand");
	float invalidScale = (float)min( TrueWidth, TrueHeight ) / 640.0f;
	// REMOVE LATER
	
	if( currentScale > maxScale ||
		( currentScale == invalidScale &&
		  currentScale != 1.0f &&
		  currentScale != 0.0f &&
		  invalidScale <  1.0f ) )
	{
		gEngfuncs.Cvar_SetValue( "barrettsniperm82a1billflxcryptedcommand", maxScale );
		gEngfuncs.Con_Printf("^3Maximum scale factor reached. Reset: %f", maxScale );
		GetScreenInfo( &m_scrinfo );
	}

	m_flScale = CVAR_GET_FLOAT( "barrettsniperm82a1billflxcryptedcommand" );

m_flScaleRadar = CVAR_GET_FLOAT( "hud_scale_radar" );

	// give a real values to other code. It's not anymore an actual CVar value
	if( m_flScale == 0.0f )
		m_flScale = 1.0f;


if( m_flScaleRadar == 0.0f )
		m_flScaleRadar = 0.90f;

	m_iRes = 640;

	// Only load this once
	if( !m_pSpriteList )
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if( m_pSpriteList )
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
			m_rghSprites      = new(std::nothrow) HSPRITE[m_iSpriteCount];
			m_rgrcRects       = new(std::nothrow) wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new(std::nothrow) char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];;

			if( !m_rghSprites || !m_rgrcRects || !m_rgszSpriteNames )
			{
				gEngfuncs.pfnConsolePrint("CHud::VidInit(): Cannot allocate memory");
				if( g_iXash && gRenderAPI.Host_Error )
					gRenderAPI.Host_Error("CHud::VidInit(): Cannot allocate memory");
			}

			p = m_pSpriteList;
			for ( int index = 0, j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
				{
					char sz[256];
					snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					char *dstName = &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH];
					strncpy(dstName, p->szName ? p->szName : "", MAX_SPRITE_NAME_LENGTH - 1);
					dstName[MAX_SPRITE_NAME_LENGTH - 1] = '\0';

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;
		int index = 0;
		for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if ( p->iRes == m_iRes )
			{
				char sz[256];
				snprintf(sz, sizeof(sz), "sprites/%s.spr", p->szSprite);
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex( "number_0" );
	m_HUD_number_small_0 = GetSpriteIndex( "number_small_0" );
	m_HUD_round_0 = GetSpriteIndex( "round_0" );


	if( m_HUD_number_0 == -1 && g_iXash && gRenderAPI.Host_Error )
	{
		gRenderAPI.Host_Error( "Failed to get number_0 sprite index. Check your game data!" );
		return;
	}

	m_iFontHeight = GetSpriteRect(m_HUD_number_0).bottom - GetSpriteRect(m_HUD_number_0).top;

	m_hGasPuff = SPR_Load("sprites/gas_puff_01.spr");


	/*m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	m_Scoreboard.VidInit();
	m_MOTD.VidInit();
	m_Timer.VidInit();
	m_Money.VidInit();
	m_ProgressBar.VidInit();
	m_SniperScope.VidInit();
	m_Radar.VidInit();
	m_SpectatorGui.VidInit();*/

	for( HUDLIST *pList = m_pHudList; pList; pList = pList->pNext )
		pList->p->VidInit();

	if( firstinit && gEngfuncs.CheckParm( "-firsttime", NULL ) )
	{
		ConsolePrint( "firstrun" );

		ClientCmd( "exec touch_presets/phone_ahsim" );
		gEngfuncs.Cvar_Set( "touch_config_file", "touch_presets/phone_ahsim.cfg" );
	}

	firstinit = false;


	const char *gameMode = CVAR_GET_STRING("mp_gamemode");

	if (!gameMode)
		gameMode = "";

	if (!strcmp(gameMode, "tdm"))
		ClientCmd("bot_all_weapons");
	else if (!strcmp(gameMode, "none"))
		ClientCmd("bot_all_weapons");
	else if (!strcmp(gameMode, "sg"))
		ClientCmd("bot_sg_only");
	else if (!strcmp(gameMode, "knife"))
		ClientCmd("bot_knives_only");
	else if (!strcmp(gameMode, "sniper"))
		ClientCmd("bot_snipers_only");
	else if (!strcmp(gameMode, "sgb"))
		ClientCmd("bot_sg_only");
	else if (!strcmp(gameMode, "sniperb"))
		ClientCmd("bot_snipers_only");

}

void CHud::Shutdown( void )
{
	for( HUDLIST *pList = m_pHudList; pList; pList = pList->pNext )
	{
		pList->p->Shutdown();
	}
}

int CHud::MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	// update Train data
	m_iLogo = reader.ReadByte();

	return 1;
}

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase(const char *in, char *out)
{
	if (!out)
		return;

	out[0] = '\0';

	if (!in || !in[0])
		return;

	const char *start = strrchr(in, '/');
	const char *start2 = strrchr(in, '\\');

	if (!start || (start2 && start2 > start))
		start = start2;

	start = start ? start + 1 : in;

	const char *end = strrchr(start, '.');

	size_t len;
	if (end && end > start)
		len = (size_t)(end - start);
	else
		len = strlen(start);

	// HUD_IsGame uses char gd[1024], so cap safely.
	if (len > 1023)
		len = 1023;

	strncpy(out, start, len);
	out[len] = '\0';
}
/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame( const char *game )
{
	const char *gamedir;
	char gd[ 1024 ];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if ( gamedir && gamedir[0] )
	{
		COM_FileBase( gamedir, gd );
		if ( !stricmp( gd, game ) )
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV( void )
{
	if ( gEngfuncs.pDemoAPI->IsRecording() )
	{
		// Write it
		unsigned char buf[ sizeof(float) ];

		// Active
		*( float * )&buf = g_lastFOV;

		Demo_WriteBuffer( TYPE_ZOOM, sizeof(float), buf );
	}

	if ( gEngfuncs.pDemoAPI->IsPlayingback() )
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

int CHud::MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf)
{
	//Weapon prediction already takes care of changing the fog. ( g_lastFOV ).
#if 0 // VALVEWHY: original client checks for "tfc" here.
	if ( cl_lw && cl_lw->value )
		return 1;
#endif

	BufferReader reader( pszName, pbuf, iSize );

	int newfov = reader.ReadByte();
	int def_fov = default_fov->value;

	g_lastFOV = newfov;
	m_iFOV = newfov ? newfov : def_fov;

	// the clients fov is actually set in the client data update section of the hud

	if ( m_iFOV == def_fov ) // reset to saved sensitivity
		m_flMouseSensitivity = 0;
	else // set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * zoom_sens_ratio->value;

	return 1;
}

void CHud::AddHudElem(CHudBase *phudelem)
{
	assert( phudelem );

	HUDLIST *pdl, *ptemp;

	pdl = new(std::nothrow) HUDLIST;
	if( !pdl )
	{
		ConsolePrint( "Cannot allocate memory!" );
		return;
	}

	pdl->p = phudelem;
	pdl->pNext = NULL;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	// find last
	for( ptemp = m_pHudList; ptemp->pNext; ptemp = ptemp->pNext );

	ptemp->pNext = pdl;
}

#define XPOS( x ) ( (x) / 16.0f )
#define YPOS( y ) ( (y) / 10.0f  )

#define INT_XPOS(x) int(XPOS(x) * ScreenWidth)
#define INT_YPOS(y) int(YPOS(y) * ScreenHeight)


//damage pb
DECLARE_MESSAGE(m_DamagePb, DamagePb);
DECLARE_COMMAND(m_DamagePb, CommandActive);

int CHudDamagePb::Init()
{
	gHUD.AddHudElem(this);
HOOK_MESSAGE(DamagePb);

HOOK_COMMAND("DamagPb", CommandActive);

	m_iFlags = HUD_DRAW;
	m_fFade = 0.0f;
	return 1;
}

int CHudDamagePb:: MsgFunc_DamagePb(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("DamagPb; HitMe");
	return 1;
}

void CHudDamagePb::UserCmd_CommandActive(void)
{
	ffade = 255;
	m_fFade = 3.0f; 
}


int CHudDamagePb::VidInit()
{
	if( R_CanLoadTexture() )
		m_damage_tex = gRenderAPI.GL_LoadTexture("gfx/billflx/damage.png", NULL, 0, TF_NEAREST|TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP);
	else
		m_damage_tex = 0;
	return 1;
}

int CHudDamagePb::Draw( float flTime )
{
	int alphaBalance;
	int alphaStatic;

	m_fFade -= gHUD.m_flTimeDelta;
	if( m_fFade <= 0 )
	{
		m_fFade = 0.0f;
		return 1;
	}

	float interpolate2 = ( 2 - m_fFade ) / 2;
	alphaBalance = 255 - interpolate2 * 255;

	if( alphaBalance < 255 )
		alphaStatic = alphaBalance;
	else
		alphaStatic = 255;

	if( R_CanBindTexture() && m_damage_tex )
	{
		gRenderAPI.GL_SelectTexture( 0 );
		gRenderAPI.GL_Bind( 0, m_damage_tex );
		gEngfuncs.pTriAPI->Color4ub( 255, 255, 255, alphaStatic );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
		DrawUtils::Draw2DQuad(
			(INT_XPOS(0) + 0) * gHUD.m_flScale,
			(INT_YPOS(0) * 0) * gHUD.m_flScale,
			(INT_XPOS(15.8) + 0 + gHUD.GetCharHeight()) * gHUD.m_flScale,
			(INT_YPOS(22) * 0.5 + gHUD.GetCharHeight()) * gHUD.m_flScale );
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}
	return 1;
}



//DeathScreen

DECLARE_MESSAGE(m_DeathScreen, DeathScreen);
DECLARE_COMMAND(m_DeathScreen, CommandActiveDeathScreen);

void CHudDeathScreen::UserCmd_CommandActiveDeathScreen(void)
{
ClientCmd("spk player/heartbeat.wav");

	ffade = 255;
	m_fFade = 3.0f; 
}

int CHudDeathScreen:: MsgFunc_DeathScreen(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("DeathScreen");
	return 1;
}

int CHudDeathScreen::Init()
{
	gHUD.AddHudElem(this);

HOOK_MESSAGE(DeathScreen);
HOOK_COMMAND("DeathScreen", CommandActiveDeathScreen);

	m_iFlags = HUD_DRAW;
	m_fFade = 0.0f;
	return 1;
}

int CHudDeathScreen::VidInit()
{
	if( R_CanLoadTexture() )
		m_death_tex = gRenderAPI.GL_LoadTexture("gfx/billflx/bloody_screen.png", NULL, 0, TF_NEAREST|TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP);
	else
		m_death_tex = 0;
	return 1;
}

int CHudDeathScreen::Draw( float flTime )
{
	int alphaBalance;
	int alphaStatic;

	m_fFade -= gHUD.m_flTimeDelta;
	if( m_fFade <= 0 )
	{
		m_fFade = 0.0f;
		return 1;
	}

	float interpolate2 = ( 2 - m_fFade ) / 2;
	alphaBalance = 255 - interpolate2 * 255;

	if( alphaBalance < 255 )
		alphaStatic = alphaBalance;
	else
		alphaStatic = 255;

	if( R_CanBindTexture() && m_death_tex )
	{
		gRenderAPI.GL_SelectTexture( 0 );
		gRenderAPI.GL_Bind( 0, m_death_tex );
		gEngfuncs.pTriAPI->Color4ub( 255, 255, 255, alphaStatic );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
		DrawUtils::Draw2DQuad(
			(INT_XPOS(0) + 0) * gHUD.m_flScale,
			(INT_YPOS(0) * 0) * gHUD.m_flScale,
			(INT_XPOS(15.8) + 0 + gHUD.GetCharHeight()) * gHUD.m_flScale,
			(INT_YPOS(22) * 0.5 + gHUD.GetCharHeight()) * gHUD.m_flScale );
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}
	return 1;
}

const char *iSec[] = {
"pbbuy weapon_colt_python",//0
"pbbuy weapon_deagle_dual",
"pbbuy weapon_dual_handgun",
"pbbuy weapon_taurus_raging_bull",
"pbbuy weapon_deagle",
"pbbuy weapon_usp",//5
"pbbuy weapon_glock18",//
"",//7//===============================================
"pbbuy weapon_bow"//8
};

const char *iPrim[] = {
"pbbuy weapon_ak47",
"pbbuy weapon_aksopmod",
"pbbuy weapon_aug_hbar", //²
"pbbuy weapon_aug", //³
"pbbuy weapon_augblitz", //4
"pbbuy weapon_p90",//5
"pbbuy weapon_aug_a3_silencer", //6
"pbbuy weapon_f2000",
"pbbuy weapon_famas_g2",
"pbbuy weapon_g36c",
"pbbuy weapon_k1",
"pbbuy weapon_k2",//11
"pbbuy weapon_kriss_sv",
"pbbuy weapon_kriss_sv_silence",
"pbbuy weapon_m4_cqb_lv1",
"pbbuy weapon_m4_cqb_lv2",
"pbbuy weapon_m4a1",//16
"pbbuy weapon_m4a1_s",
"pbbuy weapon_mp7",
"pbbuy weapon_oa93",
"pbbuy weapon_p90_mc" ,//20
"pbbuy weapon_pindad_ss2_v5",
"pbbuy weapon_groza",
"pbbuy weapon_sc2010",
"pbbuy weapon_scar_carbine",//24
"pbbuy weapon_kriss_sv_crb",
"pbbuy weapon_m4a1_s",
"pbbuy weapon_mp5k",//27
"pbbuy weapon_m4_azure",
"pbbuy weapon_mp9",
"pbbuy weapon_sg550",
//snip
"pbbuy weapon_awp",///
"pbbuy weapon_cheytac_m200",//32//
"pbbuy weapon_dragunov",//
"pbbuy weapon_kar98k",//
"pbbuy weapon_rangemaster_338",//35///
"pbbuy weapon_m82a1",//
"pbbuy weapon_tactilite_t2",//
"pbbuy weapon_scout",//38//
"pbbuy weapon_m4_spr_lv1",//
"pbbuy weapon_m4_spr_lv2",//
//sg
"pbbuy weapon_m1887",//41
"pbbuy weapon_spas_15",//
"pbbuy weapon_zombie_s",//
"pbbuy weapon_m3", //44
"", //45//===============================================
//new
"pbbuy weapon_aksopmod_cg", //46
"pbbuy weapon_aug_esport", //47
"pbbuy weapon_t77", //48
"pbbuy weapon_apc", //49
"pbbuy weapon_fg42", //50
"pbbuy weapon_msbs", //51
"pbbuy weapon_as50", //52
"pbbuy weapon_m1887_w", //53
"pbbuy weapon_pgm", //54
"pbbuy weapon_ump", //55
"pbbuy weapon_sig", //56
"pbbuy weapon_spectre", //57
"pbbuy weapon_tar", //58
"pbbuy weapon_xm8", //59
"pbbuy weapon_water" //60
};



const char *iMelee[] = {
"pbbuy weapon_knife",//0
"pbbuy weapon_amok",
"pbbuy weapon_saber",
"pbbuy weapon_arabian_sword",//³
"pbbuy weapon_fangblade",
"pbbuy weapon_combat",//5
"pbbuy weapon_knifebone",
"pbbuy weapon_brass_knuckle",
"pbbuy weapon_candy_cane",
"pbbuy weapon_dual_knife",
"pbbuy weapon_keris",
"pbbuy weapon_mini_axe",//11
"pbbuy weapon_knife",
"pbbuy weapon_ice",
"pbbuy weapon_karambit",//14
"pbbuy weapon_butterfly"
};

const char *iEx[] = {
"pbbuy weapon_hegrenade",//0
"null",//1//===============================================
"pbbuy weapon_gasbomb"//2
};

const char *iSpe[] = {
"pbbuy weapon_smokegrenade",//0
"pbbuy weapon_medkit",//1
""//===============================================
};

static std::vector<std::string> g_PrimAll;
static std::vector<std::string> g_PrimShotgun;
static std::vector<std::string> g_PrimSniper;
static std::vector<std::string> g_Sec;
static std::vector<std::string> g_Melee;
static std::vector<std::string> g_Explosive;
static std::vector<std::string> g_Special;
static bool g_WeaponListsLoaded = false;
static char g_LastWeaponListKey[64] = {0};

static bool IsEmptyOrDefaultProfile(const char *profile)
{
	if (!profile || !profile[0])
		return true;
	return !stricmp(profile, "default") || !stricmp(profile, "0");
}

static void LoadWeaponListFile(const char *filename, std::vector<std::string> &out)
{
	out.clear();

	char *buffer = (char *)gEngfuncs.COM_LoadFile(filename, 5, nullptr);
	if (!buffer)
		return;

	char token[4096];
	char *parsePos = buffer;
	while ((parsePos = gEngfuncs.COM_ParseFile(parsePos, token)))
	{
		if (!token[0])
			continue;

		// Ignore comment-like tokens
		if (token[0] == '#' || (token[0] == '/' && token[1] == '/'))
			continue;

		// Expect "weapon_xxx" tokens; store full buy command for InventoryGive.
		if (!strnicmp(token, "weapon_", 7))
		{
			std::string cmd("pbbuy ");
			cmd += token;
			out.push_back(cmd);
		}
	}

	gEngfuncs.COM_FreeFile(buffer);
}

static bool TryLoadWeaponListFile(const char *filename, std::vector<std::string> &out)
{
	out.clear();
	char *buffer = (char *)gEngfuncs.COM_LoadFile(filename, 5, nullptr);
	if (!buffer)
		return false;

	char token[4096];
	char *parsePos = buffer;
	while ((parsePos = gEngfuncs.COM_ParseFile(parsePos, token)))
	{
		if (!token[0])
			continue;
		if (token[0] == '#' || (token[0] == '/' && token[1] == '/'))
			continue;
		if (!strnicmp(token, "weapon_", 7))
		{
			std::string cmd("pbbuy ");
			cmd += token;
			out.push_back(cmd);
		}
	}

	gEngfuncs.COM_FreeFile(buffer);
	return true; // file existed (even if it contained 0 valid weapon_ tokens)
}

static void EnsureWeaponListsLoaded(void)
{
	const char *gm = CVAR_GET_STRING("mp_gamemode");
	if (!gm)
		gm = "";

	const char *profile = CVAR_GET_STRING("inv_profile");
	if (IsEmptyOrDefaultProfile(profile))
		profile = "";

	const char *key = profile[0] ? profile : gm;

	if (g_WeaponListsLoaded && !strcmp(g_LastWeaponListKey, key))
		return;

	strncpy(g_LastWeaponListKey, key, sizeof(g_LastWeaponListKey) - 1);
	g_LastWeaponListKey[sizeof(g_LastWeaponListKey) - 1] = 0;

	// Primary variants (match mp_gamemode switching in inventory UI)
	// Allow per-gamemode overrides by creating files like:
	// - weapon_list/inventory_all_<gamemode>.txt
	// - weapon_list/inventory_shotgun_<gamemode>.txt
	// - weapon_list/inventory_sniper_<gamemode>.txt
	char path[128];
	bool primAllHasFile = false;
	snprintf(path, sizeof(path), "weapon_list/inventory_all_%s.txt", key);
	primAllHasFile = TryLoadWeaponListFile(path, g_PrimAll);
	if (!primAllHasFile && !profile[0])
	{
		snprintf(path, sizeof(path), "weapon_list/inventory_all_%s.txt", gm);
		primAllHasFile = TryLoadWeaponListFile(path, g_PrimAll);
	}
	if (!primAllHasFile)
		LoadWeaponListFile("weapon_list/inventory_all.txt", g_PrimAll);

	bool primShotgunHasFile = false;
	snprintf(path, sizeof(path), "weapon_list/inventory_shotgun_%s.txt", key);
	primShotgunHasFile = TryLoadWeaponListFile(path, g_PrimShotgun);
	if (!primShotgunHasFile && !profile[0])
	{
		snprintf(path, sizeof(path), "weapon_list/inventory_shotgun_%s.txt", gm);
		primShotgunHasFile = TryLoadWeaponListFile(path, g_PrimShotgun);
	}
	if (!primShotgunHasFile)
		LoadWeaponListFile("weapon_list/inventory_shotgun.txt", g_PrimShotgun);

	bool primSniperHasFile = false;
	snprintf(path, sizeof(path), "weapon_list/inventory_sniper_%s.txt", key);
	primSniperHasFile = TryLoadWeaponListFile(path, g_PrimSniper);
	if (!primSniperHasFile && !profile[0])
	{
		snprintf(path, sizeof(path), "weapon_list/inventory_sniper_%s.txt", gm);
		primSniperHasFile = TryLoadWeaponListFile(path, g_PrimSniper);
	}
	if (!primSniperHasFile)
		LoadWeaponListFile("weapon_list/inventory_sniper.txt", g_PrimSniper);

	// Other categories
	// Per-gamemode overrides supported: inventory_secondary_<gamemode>.txt, etc.
	bool secHasFile = false;
	snprintf(path, sizeof(path), "weapon_list/inventory_secondary_%s.txt", key);
	secHasFile = TryLoadWeaponListFile(path, g_Sec);
	if (!secHasFile && !profile[0])
	{
		snprintf(path, sizeof(path), "weapon_list/inventory_secondary_%s.txt", gm);
		secHasFile = TryLoadWeaponListFile(path, g_Sec);
	}
	if (!secHasFile)
		LoadWeaponListFile("weapon_list/inventory_secondary.txt", g_Sec);

	bool meleeHasFile = false;
	snprintf(path, sizeof(path), "weapon_list/inventory_melee_%s.txt", key);
	meleeHasFile = TryLoadWeaponListFile(path, g_Melee);
	if (!meleeHasFile && !profile[0])
	{
		snprintf(path, sizeof(path), "weapon_list/inventory_melee_%s.txt", gm);
		meleeHasFile = TryLoadWeaponListFile(path, g_Melee);
	}
	if (!meleeHasFile)
		LoadWeaponListFile("weapon_list/inventory_melee.txt", g_Melee);

	bool explosiveHasFile = false;
	snprintf(path, sizeof(path), "weapon_list/inventory_explosive_%s.txt", key);
	explosiveHasFile = TryLoadWeaponListFile(path, g_Explosive);
	if (!explosiveHasFile && !profile[0])
	{
		snprintf(path, sizeof(path), "weapon_list/inventory_explosive_%s.txt", gm);
		explosiveHasFile = TryLoadWeaponListFile(path, g_Explosive);
	}
	if (!explosiveHasFile)
		LoadWeaponListFile("weapon_list/inventory_explosive.txt", g_Explosive);

	bool specialHasFile = false;
	snprintf(path, sizeof(path), "weapon_list/inventory_special_%s.txt", key);
	specialHasFile = TryLoadWeaponListFile(path, g_Special);
	if (!specialHasFile && !profile[0])
	{
		snprintf(path, sizeof(path), "weapon_list/inventory_special_%s.txt", gm);
		specialHasFile = TryLoadWeaponListFile(path, g_Special);
	}
	if (!specialHasFile)
		LoadWeaponListFile("weapon_list/inventory_special.txt", g_Special);

	g_WeaponListsLoaded = true;
}

static int WrapIndex(int index, int count)
{
	if (count <= 0)
		return 0;

	while (index < 0)
		index += count;
	while (index >= count)
		index -= count;

	return index;
}

static const std::vector<std::string> &GetPrimaryListForGamemode(void)
{
	const char *profile = CVAR_GET_STRING("inv_profile");
	if (!IsEmptyOrDefaultProfile(profile))
	{
		// Profile wins over mp_gamemode, so lobby can enforce "sniper only"
		// even when the server runs tdm/eliminate/etc.
		if (strstr(profile, "sniper"))
			return g_PrimSniper;
		if (strstr(profile, "sg") || strstr(profile, "shotgun"))
			return g_PrimShotgun;
		// For other profiles (meleeonly/primaryonly/etc), inventory_all_<profile>.txt
		// can be used to define the allowed primary list (including empty to disable).
		return g_PrimAll;
	}

	const char *gm = CVAR_GET_STRING("mp_gamemode");
	if (!gm)
		gm = "";

	if (!strcmp(gm, "sg") || !strcmp(gm, "sgb"))
		return g_PrimShotgun;
	if (!strcmp(gm, "sniper") || !strcmp(gm, "sniperB") || !strcmp(gm, "sniperb"))
		return g_PrimSniper;

	return g_PrimAll;
}

static const char *FallbackAt(const char *const *arr, int count, int index)
{
	if (!arr || count <= 0)
		return nullptr;
	if (index < 0)
		return arr[0];
	if (index >= count)
		return arr[0];
	return arr[index];
}

static void InventoryBuyFromListOrFallback(const std::vector<std::string> &list, const char *const *fallback, int fallbackCount, int index)
{
	if (!list.empty())
	{
		int wrapped = index;
		if (wrapped < 0 || wrapped >= (int)list.size())
			wrapped = 0;
		ClientCmd(list[wrapped].c_str());
		return;
	}

	const char *cmd = FallbackAt(fallback, fallbackCount, index);
	if (cmd && cmd[0])
		ClientCmd(cmd);
}

DECLARE_MESSAGE(m_InventoryGive, InventoryGive);
DECLARE_COMMAND(m_InventoryGive, CommandActiveInventoryGive);

DECLARE_MESSAGE(m_InventoryGive, FadeViewModel);
DECLARE_COMMAND(m_InventoryGive, CommandActiveFadeViewModel);

DECLARE_MESSAGE(m_InventoryGive, QuickDeploy_client);
DECLARE_COMMAND(m_InventoryGive, CommandActiveQuickDeploy_client);

DECLARE_MESSAGE(m_InventoryGive, QuickReload_client);
DECLARE_COMMAND(m_InventoryGive, CommandActiveQuickReload_client);

DECLARE_MESSAGE(m_InventoryGive, BlinkViewmodel);
DECLARE_COMMAND(m_InventoryGive, CommandActiveBlinkViewmodel);

DECLARE_MESSAGE(m_InventoryGive, Mode_ui);
DECLARE_COMMAND(m_InventoryGive, CommandActiveMode_ui);

void CHudInventoryGive::UserCmd_CommandActiveQuickDeploy_client(void)
{
if (gHUD.item_QuickDeploy->value)
ClientCmd("billflxcrypted_quickdraw");
}

void CHudInventoryGive::UserCmd_CommandActiveQuickReload_client(void)
{
if (gHUD.item_QuickReload->value)
ClientCmd("billflxcrypted_quickreload");
}

void CHudInventoryGive::UserCmd_CommandActiveMode_ui(void)
{
	const char *gameMode = CVAR_GET_STRING("mp_gamemode");
	if (!gameMode)
		gameMode = "";

	if (!strcmp(gameMode, "tdm"))
		ClientCmd("exec inv/inv_tdm; bot_all_weapons");
	else if (!strcmp(gameMode, "none"))
		ClientCmd("exec inv/inv_tdm; bot_all_weapons");
	else if (!strcmp(gameMode, "sg"))
		ClientCmd("exec inv/inv_sg_tdm; bot_sg_only");
	else if (!strcmp(gameMode, "knife"))
		ClientCmd("exec inv/inv_knife_tdm; bot_knives_only");
	else if (!strcmp(gameMode, "sniper"))
		ClientCmd("exec inv/inv_sniper_tdm; bot_snipers_only");
	else if (!strcmp(gameMode, "sgb"))
		ClientCmd("exec inv/inv_sg_bomb; bot_sg_only");
	else if (!strcmp(gameMode, "sniperb"))
		ClientCmd("exec inv/inv_sniper_bomb; bot_snipers_only");
	else if (!strcmp(gameMode, "knifeB") || !strcmp(gameMode, "knifeb"))
		ClientCmd("exec inv/inv_knife_bomb; bot_knives_only");
}

void CHudInventoryGive::UserCmd_CommandActiveInventoryGive(void) //on spawn
{

gHUD.slugger_count = 0;
gHUD.Announcement_Red = FALSE;
gHUD.Announcement_Blue = FALSE;
gHUD.respawning = FALSE;

EnsureWeaponListsLoaded();

int counterPrim = gHUD.inventory_primary->value;
int counterSec = gHUD.inventory_secondary->value;
int counterMelee = gHUD.inventory_melee->value;
int counterHegren = gHUD.inventory_explosive->value;
int counterSp = gHUD.inventory_special->value;

gHUD.helmet_on = TRUE;//GIVE ME HELMET

const std::vector<std::string> &primList = GetPrimaryListForGamemode();
InventoryBuyFromListOrFallback(primList, iPrim, (int)(sizeof(iPrim) / sizeof(iPrim[0])), counterPrim);
InventoryBuyFromListOrFallback(g_Sec, iSec, (int)(sizeof(iSec) / sizeof(iSec[0])), counterSec);
InventoryBuyFromListOrFallback(g_Melee, iMelee, (int)(sizeof(iMelee) / sizeof(iMelee[0])), counterMelee);
InventoryBuyFromListOrFallback(g_Explosive, iEx, (int)(sizeof(iEx) / sizeof(iEx[0])), counterHegren);
InventoryBuyFromListOrFallback(g_Special, iSpe, (int)(sizeof(iSpe) / sizeof(iSpe[0])), counterSp);

ClientCmd("BlinkViewmodel");

gEngfuncs.Cvar_SetValue( "billflxstar1cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar2cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar3cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar4cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar5cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar6cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar7cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar8cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar9cmd", 0);
gEngfuncs.Cvar_SetValue( "billflxstar10cmd", 0);

}

void CHudInventoryGive::UserCmd_CommandActiveFadeViewModel(void)
{
FadeViewModel_time = 20.0f;
}

void CHudInventoryGive::UserCmd_CommandActiveBlinkViewmodel(void) 
{
BlinkViewmodel_time = 300.0f;
}

int CHudInventoryGive:: MsgFunc_InventoryGive(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("InventoryGive");
	return 1;
}

int CHudInventoryGive:: MsgFunc_BlinkViewmodel(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("BlinkViewmodel");
	return 1;
}

int CHudInventoryGive:: MsgFunc_QuickDeploy_client(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("QuickDeploy_client");
	return 1;
}

int CHudInventoryGive:: MsgFunc_QuickReload_client(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("QuickReload_client");
	return 1;
}

int CHudInventoryGive:: MsgFunc_FadeViewModel(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("FadeViewModel");
	return 1;
}

int CHudInventoryGive:: MsgFunc_Mode_ui(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Mode_ui");
	return 1;
}

int CHudInventoryGive::Init()
{
HOOK_MESSAGE(InventoryGive);
HOOK_COMMAND("InventoryGive", CommandActiveInventoryGive);

HOOK_MESSAGE(QuickDeploy_client);
HOOK_COMMAND("QuickDeploy_client", CommandActiveQuickDeploy_client);

HOOK_MESSAGE(QuickReload_client);
HOOK_COMMAND("QuickReload_client", CommandActiveQuickReload_client);

HOOK_MESSAGE(FadeViewModel);
HOOK_COMMAND("FadeViewModel", CommandActiveFadeViewModel);

HOOK_MESSAGE(BlinkViewmodel);
HOOK_COMMAND("BlinkViewmodel", CommandActiveBlinkViewmodel);

HOOK_MESSAGE(Mode_ui);
HOOK_COMMAND("Mode_ui", CommandActiveMode_ui);

gHUD.AddHudElem(this);
m_iFlags = HUD_DRAW;

return 1;
}


int CHudInventoryGive::VidInit()
{
return 1;
}

int CHudInventoryGive::Draw( float flTime )
{

FadeViewModel_time -= 1;
BlinkViewmodel_time -= 1;

if (BlinkViewmodel_time == 300) {/*ClientCmd("r_drawviewmodel 0");*/}
else if (BlinkViewmodel_time == 299) { //respawn items

if(gHUD.item_MegaHp->value)
{
ClientCmd("billflxcrypted_megahp");
gHUD.megahp = TRUE;
}
else
{
gHUD.megahp = FALSE;
}

if (gHUD.item_mask->value == 0)
{
ClientCmd("billflxcrypted_mask0");
}
else if (gHUD.item_mask->value == 1)
{
ClientCmd("billflxcrypted_mask1");
}
else if (gHUD.item_mask->value == 2)
{
ClientCmd("billflxcrypted_mask2");
}
else if (gHUD.item_mask->value == 3)
{
ClientCmd("billflxcrypted_mask3");
}
else if (gHUD.item_mask->value == 4)
{
ClientCmd("billflxcrypted_mask4");
}
else if (gHUD.item_mask->value == 5)
{
ClientCmd("billflxcrypted_mask5");
}
else if (gHUD.item_mask->value == 6)
{
ClientCmd("billflxcrypted_mask6");
}
else if (gHUD.item_mask->value == 7)
{
ClientCmd("billflxcrypted_mask7");
}
else if (gHUD.item_mask->value == 8)
{
ClientCmd("billflxcrypted_mask8");
}
else if (gHUD.item_mask->value == 9)
{
ClientCmd("billflxcrypted_mask9");
}
else if (gHUD.item_mask->value == 10)
{
ClientCmd("billflxcrypted_mask10");
}
else if (gHUD.item_mask->value == 11)
{
ClientCmd("billflxcrypted_mask11");
}
else if (gHUD.item_mask->value == 12)
{
ClientCmd("billflxcrypted_mask12");
}
else if (gHUD.item_mask->value == 13)
{
ClientCmd("billflxcrypted_mask13");
}
} 
else if (BlinkViewmodel_time == 298) {} 

return 1;
}

////keys
DECLARE_MESSAGE(m_Keys, ShowQ);
DECLARE_COMMAND(m_Keys, CommandActiveShowQ);

int CHudKeys::MsgFunc_ShowQ(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("ShowQ");
	return 1;
}

int CHudKeys::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = HUD_DRAW;

HOOK_MESSAGE(ShowQ);
HOOK_COMMAND("ShowQ", CommandActiveShowQ);

	return 1;
}

void CHudKeys::UserCmd_CommandActiveShowQ(void)  
{ Q_time = 10.0f; }

int CHudKeys::VidInit()
{
	R_InitTexture(m_keys[0], "gfx/billflx/k_overlay/a.png");
	R_InitTexture(m_keys[1], "gfx/billflx/k_overlay/attack.png");
	R_InitTexture(m_keys[2], "gfx/billflx/k_overlay/attack2.png");
	R_InitTexture(m_keys[3], "gfx/billflx/k_overlay/bg.png");
	R_InitTexture(m_keys[4], "gfx/billflx/k_overlay/change.png");
	R_InitTexture(m_keys[5], "gfx/billflx/k_overlay/crouch.png");
	R_InitTexture(m_keys[6], "gfx/billflx/k_overlay/d.png");
	R_InitTexture(m_keys[7], "gfx/billflx/k_overlay/r.png");
	R_InitTexture(m_keys[8], "gfx/billflx/k_overlay/s.png");
	R_InitTexture(m_keys[9], "gfx/billflx/k_overlay/score.png");
	R_InitTexture(m_keys[10], "gfx/billflx/k_overlay/space.png");
	R_InitTexture(m_keys[11], "gfx/billflx/k_overlay/w.png");
	return 1;
}

int CHudKeys::Draw(float flTime)
{
Q_time -= 1;

int HealthWidth;
int HealthHeight;
int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
int x = ScreenWidth / 6;

HealthHeight = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).bottom - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).top;
HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

if (gHUD.enable_key_overlay->value)
{

gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
gEngfuncs.pTriAPI->Brightness(1.0);
gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);

m_keys[3]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);


if (Q_time == 10) { m_keys[4]->Bind(); DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight); }
else if (Q_time == 9) { m_keys[4]->Bind(); DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight); }
else if (Q_time == 8) { m_keys[4]->Bind(); DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight); }
else if (Q_time == 7) { m_keys[4]->Bind(); DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight); }
else if (Q_time == 6) { m_keys[4]->Bind(); DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight); }
else if (Q_time == 5) { m_keys[4]->Bind(); DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight); }

if (gHUD.key_tab)
{
m_keys[9]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}

if (gHUD.m_iKeyBits & IN_FORWARD)
{
m_keys[11]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_MOVELEFT)
{
m_keys[0]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_MOVERIGHT)
{
m_keys[6]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_BACK)
{
m_keys[8]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_JUMP)
{
m_keys[10]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_DUCK)
{
m_keys[5]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_RELOAD)
{
m_keys[7]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_SCORE)
{
m_keys[9]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_ATTACK)
{
m_keys[1]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}
if (gHUD.m_iKeyBits & IN_ATTACK2)
{
m_keys[2]->Bind();
DrawUtils::Draw2DQuad(x - HealthWidth, y - gHUD.m_iFontHeight / 0.17, x - HealthWidth + ScreenWidth / 5 - HealthWidth, ScreenHeight);
}

}
	return 0;
}


DECLARE_MESSAGE(m_HealthBar, HealthBar);
DECLARE_COMMAND(m_HealthBar, CommandActiveHealthBar);

void CHudHealthbar::UserCmd_CommandActiveHealthBar(void)
{
m_fFade = 1.0f;
}

int CHudHealthbar::MsgFunc_HealthBar(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ehealth = reader.ReadByte();
	instantDeath = reader.ReadByte();

float x = reader.ReadCoord();
float y = reader.ReadCoord();
float z = reader.ReadCoord();
    
m_TargetOrigin.x = x;
m_TargetOrigin.y = y;
m_TargetOrigin.z = z;

	ClientCmd("HealthBar");
	return 1;
}

int CHudHealthbar::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = HUD_DRAW;

HOOK_MESSAGE(HealthBar);
HOOK_COMMAND("HealthBar", CommandActiveHealthBar);

	return 1;
}

int CHudHealthbar::VidInit()
{

R_InitTexture(m_healthbar[11], "gfx/billflx/hb/bg");
R_InitTexture(m_healthbar[10], "gfx/billflx/hb/100");
R_InitTexture(m_healthbar[9], "gfx/billflx/hb/90");
R_InitTexture(m_healthbar[8], "gfx/billflx/hb/80");
R_InitTexture(m_healthbar[7], "gfx/billflx/hb/70");
R_InitTexture(m_healthbar[6], "gfx/billflx/hb/60");
R_InitTexture(m_healthbar[5], "gfx/billflx/hb/50");
R_InitTexture(m_healthbar[4], "gfx/billflx/hb/40");
R_InitTexture(m_healthbar[3], "gfx/billflx/hb/30");
R_InitTexture(m_healthbar[2], "gfx/billflx/hb/20");
R_InitTexture(m_healthbar[1], "gfx/billflx/hb/10");
R_InitTexture(m_healthbar[0], "gfx/billflx/hb/0");

return 1;
}

int CHudHealthbar::Draw( float flTime )
{
if (gHUD.enable_healthbar_overlay->value == 0)
return 0;

int x, y;

Vector screen;
gEngfuncs.pTriAPI->WorldToScreen(m_TargetOrigin, screen);
y = (0.7f - screen.y) * (ScreenHeight * 0.2f);
x = (1.0f + screen.x) * ScreenWidth * 0.5f;

int alphaBalance;
int alphaStatic;
m_fFade -= gHUD.m_flTimeDelta;

if( m_fFade < 0)
{
m_fFade = 0.0f;
}

float interpolate2 = ( 2 - m_fFade ) / 2;
alphaBalance = 255 - interpolate2 * 255;

if(alphaBalance < 255)
alphaStatic = alphaBalance;
else
alphaStatic = 255;


if (instantDeath == 1)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else
{
if (ehealth <= 0)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 10)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[1]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 20)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[2]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 30)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[3]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 40)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[4]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 50)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[5]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 60)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[6]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 70)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[7]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 80)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[8]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 90)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[9]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 100)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[10]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}
else if (ehealth <= 130)
{
gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(0, 0, 0, alphaStatic);
m_healthbar[11]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);

gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
gEngfuncs.pTriAPI->Color4ub(255, 10, 10, alphaStatic);
m_healthbar[10]->Bind();
DrawUtils::Draw2DQuadScaled(x - 100 / 2,y + 200, x + 100 / 2, y + 210);
}

}

return 0;
}

int CHudHealthbar::DrawBar(int x, int y, int width, int height, float f, int& r, int& g, int& b, int& a)
{

	f = bound(0, f, 1);
	int w = f * width;
	if (f > 0.25)
	{
		// Always show at least one pixel if we have ammo.
		FillRGBA(x, y, w, height, r, g, b, a);
		x += w;
		width -= w;
	}
	else
	{
		if (w <= 0)
			w = 1;
		FillRGBA(x, y, w, height, r, g, b, a);
		x += w;
		width -= w;
	}

	return (x + width);
}


//inventory ui
DECLARE_MESSAGE(m_InventoryUi, Openinv);
DECLARE_COMMAND(m_InventoryUi, CommandActiveOpeninv);

DECLARE_MESSAGE(m_InventoryUi, Closeinv);
DECLARE_COMMAND(m_InventoryUi, CommandActiveCloseinv);

DECLARE_MESSAGE(m_InventoryUi, Nextprim);
DECLARE_COMMAND(m_InventoryUi, CommandActiveNextprim);
DECLARE_MESSAGE(m_InventoryUi, Prevprim);
DECLARE_COMMAND(m_InventoryUi, CommandActivePrevprim);

DECLARE_MESSAGE(m_InventoryUi, Nextsec);
DECLARE_COMMAND(m_InventoryUi, CommandActiveNextsec);
DECLARE_MESSAGE(m_InventoryUi, Prevsec);
DECLARE_COMMAND(m_InventoryUi, CommandActivePrevsec);

DECLARE_MESSAGE(m_InventoryUi, Nextmelee);
DECLARE_COMMAND(m_InventoryUi, CommandActiveNextmelee);
DECLARE_MESSAGE(m_InventoryUi, Prevmelee);
DECLARE_COMMAND(m_InventoryUi, CommandActivePrevmelee);

DECLARE_MESSAGE(m_InventoryUi, Nextexplo);
DECLARE_COMMAND(m_InventoryUi, CommandActiveNextexplo);
DECLARE_MESSAGE(m_InventoryUi, Prevexplo);
DECLARE_COMMAND(m_InventoryUi, CommandActivePrevexplo);

DECLARE_MESSAGE(m_InventoryUi, Nextspe);
DECLARE_COMMAND(m_InventoryUi, CommandActiveNextspe);
DECLARE_MESSAGE(m_InventoryUi, Prevspe);
DECLARE_COMMAND(m_InventoryUi, CommandActivePrevspe);

//prim
void CHudInventoryUi::UserCmd_CommandActivePrevprim(void)
{

EnsureWeaponListsLoaded();

if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "none") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "tdm"))
{
gHUD.inventory_primary->value -= 1;

int count = !g_PrimAll.empty() ? (int)g_PrimAll.size() : (int)(sizeof(iPrim) / sizeof(iPrim[0]));
gHUD.inventory_primary->value = WrapIndex((int)gHUD.inventory_primary->value, count);
}
else if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "sg") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "sgb"))
{
gHUD.inventory_primary->value -= 1;

int count = !g_PrimShotgun.empty() ? (int)g_PrimShotgun.size() : 4;
gHUD.inventory_primary->value = WrapIndex((int)gHUD.inventory_primary->value, count);
}
else if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "sniper") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "sniperB"))
{
gHUD.inventory_primary->value -= 1;

int count = !g_PrimSniper.empty() ? (int)g_PrimSniper.size() : 10;
gHUD.inventory_primary->value = WrapIndex((int)gHUD.inventory_primary->value, count);
}

gEngfuncs.Cvar_SetValue( "inventory_primary", gHUD.inventory_primary->value );
}

void CHudInventoryUi::UserCmd_CommandActiveNextprim(void)
{

EnsureWeaponListsLoaded();

if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "none") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "tdm"))
{
gHUD.inventory_primary->value += 1;

int count = !g_PrimAll.empty() ? (int)g_PrimAll.size() : (int)(sizeof(iPrim) / sizeof(iPrim[0]));
gHUD.inventory_primary->value = WrapIndex((int)gHUD.inventory_primary->value, count);
}
else if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "sg") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "sgb"))
{
gHUD.inventory_primary->value += 1;

int count = !g_PrimShotgun.empty() ? (int)g_PrimShotgun.size() : 4;
gHUD.inventory_primary->value = WrapIndex((int)gHUD.inventory_primary->value, count);
}
else if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "sniper") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "sniperB"))
{
gHUD.inventory_primary->value += 1;

int count = !g_PrimSniper.empty() ? (int)g_PrimSniper.size() : 10;
gHUD.inventory_primary->value = WrapIndex((int)gHUD.inventory_primary->value, count);
}

gEngfuncs.Cvar_SetValue( "inventory_primary", gHUD.inventory_primary->value );
}

//sec
void CHudInventoryUi::UserCmd_CommandActivePrevsec(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_secondary->value -= 1;

int count = !g_Sec.empty() ? (int)g_Sec.size() : (int)(sizeof(iSec) / sizeof(iSec[0]));
gHUD.inventory_secondary->value = WrapIndex((int)gHUD.inventory_secondary->value, count);

gEngfuncs.Cvar_SetValue( "inventory_secondary", gHUD.inventory_secondary->value );
}

void CHudInventoryUi::UserCmd_CommandActiveNextsec(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_secondary->value += 1;

int count = !g_Sec.empty() ? (int)g_Sec.size() : (int)(sizeof(iSec) / sizeof(iSec[0]));
gHUD.inventory_secondary->value = WrapIndex((int)gHUD.inventory_secondary->value, count);

gEngfuncs.Cvar_SetValue( "inventory_secondary", gHUD.inventory_secondary->value );
}

//melee
void CHudInventoryUi::UserCmd_CommandActivePrevmelee(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_melee->value -= 1;

int count = !g_Melee.empty() ? (int)g_Melee.size() : (int)(sizeof(iMelee) / sizeof(iMelee[0]));
gHUD.inventory_melee->value = WrapIndex((int)gHUD.inventory_melee->value, count);

gEngfuncs.Cvar_SetValue( "inventory_melee", gHUD.inventory_melee->value );
}
void CHudInventoryUi::UserCmd_CommandActiveNextmelee(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_melee->value += 1;

int count = !g_Melee.empty() ? (int)g_Melee.size() : (int)(sizeof(iMelee) / sizeof(iMelee[0]));
gHUD.inventory_melee->value = WrapIndex((int)gHUD.inventory_melee->value, count);

gEngfuncs.Cvar_SetValue( "inventory_melee", gHUD.inventory_melee->value );
}

//explosive 
void CHudInventoryUi::UserCmd_CommandActivePrevexplo(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_explosive->value -= 1;

int count = !g_Explosive.empty() ? (int)g_Explosive.size() : (int)(sizeof(iEx) / sizeof(iEx[0]));
gHUD.inventory_explosive->value = WrapIndex((int)gHUD.inventory_explosive->value, count);

gEngfuncs.Cvar_SetValue( "inventory_explosive", gHUD.inventory_explosive->value );
}
void CHudInventoryUi::UserCmd_CommandActiveNextexplo(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_explosive->value += 1;

int count = !g_Explosive.empty() ? (int)g_Explosive.size() : (int)(sizeof(iEx) / sizeof(iEx[0]));
gHUD.inventory_explosive->value = WrapIndex((int)gHUD.inventory_explosive->value, count);

gEngfuncs.Cvar_SetValue( "inventory_explosive", gHUD.inventory_explosive->value );
}

//special 
void CHudInventoryUi::UserCmd_CommandActivePrevspe(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_special->value -= 1;

int count = !g_Special.empty() ? (int)g_Special.size() : (int)(sizeof(iSpe) / sizeof(iSpe[0]));
gHUD.inventory_special->value = WrapIndex((int)gHUD.inventory_special->value, count);

gEngfuncs.Cvar_SetValue( "inventory_special", gHUD.inventory_special->value );
}
void CHudInventoryUi::UserCmd_CommandActiveNextspe(void)
{
EnsureWeaponListsLoaded();
gHUD.inventory_special->value += 1;

int count = !g_Special.empty() ? (int)g_Special.size() : (int)(sizeof(iSpe) / sizeof(iSpe[0]));
gHUD.inventory_special->value = WrapIndex((int)gHUD.inventory_special->value, count);

gEngfuncs.Cvar_SetValue( "inventory_special", gHUD.inventory_special->value );
}

//close and open default 
void CHudInventoryUi::UserCmd_CommandActiveOpeninv(void)
{
open = TRUE;
//m_fFade = 1.0f;

if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "knife") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "knifeB") )
{
gEngfuncs.Cvar_SetValue( "inventory_primary", 45); //no rifles allowed brother 
gEngfuncs.Cvar_SetValue( "inventory_secondary", 7);
}

}

void CHudInventoryUi::UserCmd_CommandActiveCloseinv(void)
{
open = FALSE;
m_fFade = 1.0f;


if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "knife") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "knifeB") )
{
gEngfuncs.Cvar_SetValue( "inventory_primary", 45); //no rifles allowed brother 
gEngfuncs.Cvar_SetValue( "inventory_secondary", 7);
}
}


int CHudInventoryUi::MsgFunc_Openinv(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Openinv");
	return 1;
}
int CHudInventoryUi::MsgFunc_Closeinv(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Closeinv");
	return 1;
}


int CHudInventoryUi::MsgFunc_Nextprim(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Nextprim");
	return 1;
}

int CHudInventoryUi::MsgFunc_Prevprim(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Prevprim");
	return 1;
}

int CHudInventoryUi::MsgFunc_Nextsec(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Nextsec");
	return 1;
}

int CHudInventoryUi::MsgFunc_Prevsec(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Prevsec");
	return 1;
}

int CHudInventoryUi::MsgFunc_Nextmelee(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Nextmelee");
	return 1;
}

int CHudInventoryUi::MsgFunc_Prevmelee(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Prevmelee");
	return 1;
}

int CHudInventoryUi::MsgFunc_Nextexplo(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Nextexplo");
	return 1;
}

int CHudInventoryUi::MsgFunc_Prevexplo(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Prevexplo");
	return 1;
}

int CHudInventoryUi::MsgFunc_Nextspe(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Nextspe");
	return 1;
}

int CHudInventoryUi::MsgFunc_Prevspe(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	ClientCmd("Prevspe");
	return 1;
}



int CHudInventoryUi::Init()
{
gHUD.AddHudElem(this);

HOOK_MESSAGE(Openinv);
HOOK_COMMAND("Openinv", CommandActiveOpeninv);
HOOK_MESSAGE(Closeinv);
HOOK_COMMAND("Closeinv", CommandActiveCloseinv);


HOOK_MESSAGE(Nextprim);
HOOK_COMMAND("Nextprim", CommandActiveNextprim);
HOOK_MESSAGE(Prevprim);
HOOK_COMMAND("Prevprim", CommandActivePrevprim);

HOOK_MESSAGE(Nextsec);
HOOK_COMMAND("Nextsec", CommandActiveNextsec);
HOOK_MESSAGE(Prevsec);
HOOK_COMMAND("Prevsec", CommandActivePrevsec);

HOOK_MESSAGE(Nextmelee);
HOOK_COMMAND("Nextmelee", CommandActiveNextmelee);
HOOK_MESSAGE(Prevmelee);
HOOK_COMMAND("Prevmelee", CommandActivePrevmelee);

HOOK_MESSAGE(Nextexplo);
HOOK_COMMAND("Nextexplo", CommandActiveNextexplo);
HOOK_MESSAGE(Prevexplo);
HOOK_COMMAND("Prevexplo", CommandActivePrevexplo);

HOOK_MESSAGE(Nextspe);
HOOK_COMMAND("Nextspe", CommandActiveNextspe);
HOOK_MESSAGE(Prevspe);
HOOK_COMMAND("Prevspe", CommandActivePrevspe);

m_iFlags = HUD_DRAW;
return 1;
}


int CHudInventoryUi::VidInit()
{
R_InitTexture(bg, "gfx/ui/inv_bg.png");

R_InitTexture(primary[0], "gfx/billflx/weapons/wpn_empty.png");
R_InitTexture(primary[1], "gfx/billflx/weapons/870mcs.png");
R_InitTexture(primary[2], "gfx/billflx/weapons/ak47.png");
R_InitTexture(primary[3], "gfx/billflx/weapons/aksopmod.png");
R_InitTexture(primary[4], "gfx/billflx/weapons/aksopmodcg.png");
R_InitTexture(primary[5], "gfx/billflx/weapons/apc.png");
R_InitTexture(primary[6], "gfx/billflx/weapons/as50.png");
R_InitTexture(primary[7], "gfx/billflx/weapons/aug.png");
R_InitTexture(primary[8], "gfx/billflx/weapons/augblitz.png");
R_InitTexture(primary[9], "gfx/billflx/weapons/aughbar.png");
R_InitTexture(primary[10], "gfx/billflx/weapons/aug_silencer.png");
R_InitTexture(primary[11], "gfx/billflx/weapons/cheytac.png");
R_InitTexture(primary[12], "gfx/billflx/weapons/dragunov.png");
R_InitTexture(primary[13], "gfx/billflx/weapons/f2000.png");
R_InitTexture(primary[14], "gfx/billflx/weapons/famas.png");
R_InitTexture(primary[15], "gfx/billflx/weapons/fg42.png");
R_InitTexture(primary[16], "gfx/billflx/weapons/g36c.png");
R_InitTexture(primary[17], "gfx/billflx/weapons/groza.png");
R_InitTexture(primary[18], "gfx/billflx/weapons/hk417.png");
R_InitTexture(primary[19], "gfx/billflx/weapons/k1.png");
R_InitTexture(primary[20], "gfx/billflx/weapons/k2.png");
R_InitTexture(primary[21], "gfx/billflx/weapons/kar98k.png");
R_InitTexture(primary[22], "gfx/billflx/weapons/krissbatik.png");
R_InitTexture(primary[23], "gfx/billflx/weapons/kriss_crb.png");
R_InitTexture(primary[24], "gfx/billflx/weapons/kriss_silence.png");
R_InitTexture(primary[25], "gfx/billflx/weapons/kriss_sv.png");
R_InitTexture(primary[26], "gfx/billflx/weapons/l115a1.png");
R_InitTexture(primary[27], "gfx/billflx/weapons/m4a1.png");
R_InitTexture(primary[28], "gfx/billflx/weapons/m4a1_s.png");
R_InitTexture(primary[29], "gfx/billflx/weapons/m4_cqb_lv1.png");
R_InitTexture(primary[30], "gfx/billflx/weapons/m4_cqb_lv2.png");
R_InitTexture(primary[31], "gfx/billflx/weapons/m4_spr_lv1.png");
R_InitTexture(primary[32], "gfx/billflx/weapons/m4_spr_lv2.png");
R_InitTexture(primary[33], "gfx/billflx/weapons/m82a1.png");
R_InitTexture(primary[34], "gfx/billflx/weapons/m1887.png");
R_InitTexture(primary[35], "gfx/billflx/weapons/m1887w.png");
R_InitTexture(primary[36], "gfx/billflx/weapons/mp5k.png");
R_InitTexture(primary[37], "gfx/billflx/weapons/mp7.png");
R_InitTexture(primary[38], "gfx/billflx/weapons/mp9.png");
R_InitTexture(primary[39], "gfx/billflx/weapons/msbs.png");
R_InitTexture(primary[40], "gfx/billflx/weapons/oa93.png");
R_InitTexture(primary[41], "gfx/billflx/weapons/p90.png");
R_InitTexture(primary[42], "gfx/billflx/weapons/p90mc.png");
R_InitTexture(primary[43], "gfx/billflx/weapons/pgm.png");
R_InitTexture(primary[44], "gfx/billflx/weapons/pindad_ss2_v5.png");
R_InitTexture(primary[45], "gfx/billflx/weapons/rangemaster338.png");
R_InitTexture(primary[46], "gfx/billflx/weapons/sc2010.png");
R_InitTexture(primary[47], "gfx/billflx/weapons/scar_carbine.png");
R_InitTexture(primary[48], "gfx/billflx/weapons/sg550.png");
R_InitTexture(primary[49], "gfx/billflx/weapons/sig_sauer.png");
R_InitTexture(primary[50], "gfx/billflx/weapons/spas_15.png");
R_InitTexture(primary[51], "gfx/billflx/weapons/spectre.png");
R_InitTexture(primary[52], "gfx/billflx/weapons/ssg69.png");
R_InitTexture(primary[53], "gfx/billflx/weapons/t77.png");
R_InitTexture(primary[54], "gfx/billflx/weapons/tactilite_t2.png");
R_InitTexture(primary[55], "gfx/billflx/weapons/tar21.png");
R_InitTexture(primary[56], "gfx/billflx/weapons/ump.png");
R_InitTexture(primary[57], "gfx/billflx/weapons/watergun.png");
R_InitTexture(primary[58], "gfx/billflx/weapons/xm8.png");
R_InitTexture(primary[59], "gfx/billflx/weapons/zombie_slayer.png");

R_InitTexture(secondary[0], "gfx/billflx/weapons/wpn_empty.png");
R_InitTexture(secondary[1], "gfx/billflx/weapons/raging_bull.png");
R_InitTexture(secondary[2], "gfx/billflx/weapons/glock.png");
R_InitTexture(secondary[3], "gfx/billflx/weapons/k5.png");
R_InitTexture(secondary[4], "gfx/billflx/weapons/desert_eagle.png");
R_InitTexture(secondary[5], "gfx/billflx/weapons/desert_eagle_dual.png");
R_InitTexture(secondary[6], "gfx/billflx/weapons/coltpython.png");
R_InitTexture(secondary[7], "gfx/billflx/weapons/dualhandgun.png");
R_InitTexture(secondary[8], "gfx/billflx/weapons/bow.png");

R_InitTexture(melee[0], "gfx/billflx/weapons/wpn_empty.png");
R_InitTexture(melee[1], "gfx/billflx/weapons/saber.png");
R_InitTexture(melee[2], "gfx/billflx/weapons/miniaxe.png");
R_InitTexture(melee[3], "gfx/billflx/weapons/m7.png");
R_InitTexture(melee[4], "gfx/billflx/weapons/karambit.png");
R_InitTexture(melee[5], "gfx/billflx/weapons/keris.png");
R_InitTexture(melee[6], "gfx/billflx/weapons/knight_sword.png");
R_InitTexture(melee[7], "gfx/billflx/weapons/fangblade.png");
R_InitTexture(melee[8], "gfx/billflx/weapons/dual_knife.png");
R_InitTexture(melee[9], "gfx/billflx/weapons/combat.png");
R_InitTexture(melee[10], "gfx/billflx/weapons/amok.png");
R_InitTexture(melee[11], "gfx/billflx/weapons/bone_knife.png");
R_InitTexture(melee[12], "gfx/billflx/weapons/brass_knuckle.png");
R_InitTexture(melee[13], "gfx/billflx/weapons/butterfly.png");
R_InitTexture(melee[14], "gfx/billflx/weapons/candy_cane.png");
R_InitTexture(melee[15], "gfx/billflx/weapons/arabian_sword.png");

R_InitTexture(explosive[0], "gfx/billflx/weapons/wpn_empty.png");
R_InitTexture(explosive[1], "gfx/billflx/weapons/k400.png");
R_InitTexture(explosive[2], "gfx/billflx/weapons/gasbomb.png");
R_InitTexture(explosive[3], "gfx/billflx/weapons/gasbomb.png");
R_InitTexture(explosive[4], "gfx/billflx/weapons/gasbomb.png");

R_InitTexture(special[0], "gfx/billflx/weapons/wpn_empty.png");
R_InitTexture(special[1], "gfx/billflx/weapons/smoke.png");
R_InitTexture(special[2], "gfx/billflx/weapons/medkit.png");
R_InitTexture(special[3], "gfx/billflx/weapons/medkit.png");
R_InitTexture(special[4], "gfx/billflx/weapons/medkit.png");

return 1;
}

int CHudInventoryUi::Draw( float flTime )
{
	int x = ScreenWidth / 2;
	int xr = ScreenWidth / 1.58;
	int xl = ScreenWidth / 2;

	int primXPos = 0;

	if (ScreenHeight <= 600) primXPos = 110;
	else if (ScreenHeight <= 700) primXPos = 130;
	else if (ScreenHeight <= 800) primXPos = 160;
	else if (ScreenHeight <= 900) primXPos = 190;
	else if (ScreenHeight <= 1100) primXPos = 220;
	else if (ScreenHeight <= 1500) primXPos = 310;
	else if (ScreenHeight <= 2500) primXPos = 330;

	if (open)
	{
		//bg
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
		gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
		bg->Bind();
		DrawUtils::Draw2DQuadScaled(x - 1000 / 2, 0, x + 1000 / 2, ScreenHeight);

		if (!strcmp(CVAR_GET_STRING("mp_gamemode"), "none") || !strcmp(CVAR_GET_STRING("mp_gamemode"), "tdm"))
		{
			// Render weapons if they exist
			if (primary[1]) {
				primary[1]->Bind();
				DrawUtils::Draw2DQuadScaled(x - 300 / 2, primXPos, x + 300 / 2, primXPos + 100);
			}
			if (primary[2]) {
				primary[2]->Bind();
				DrawUtils::Draw2DQuadScaled(xr - 300 / 2, primXPos + 10, xr + 300 / 2, primXPos + 100 + 10);
			}
		}
	}
	return 1;
}
