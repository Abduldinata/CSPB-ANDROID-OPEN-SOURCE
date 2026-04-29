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
// death notice
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include "draw_util.h"

#include "triangleapi.h"

float color[3];

DECLARE_MESSAGE( m_DeathNotice, DeathMsg )

enum DrawBgType
{
	DB_NONE, 
	DB_KILL,
	DB_DEATH
};

struct DeathNoticeItem 
{
	char szKiller[MAX_PLAYER_NAME_LENGTH*2];
	char szVictim[MAX_PLAYER_NAME_LENGTH*2];
	int iId;	// the index number of the associated sprite
	bool bSuicide;
	bool bTeamKill;
	bool bNonPlayerKill;
	float flDisplayTime;
	float flStartTime; // Added for animation
	float *KillerColor;
	float *VictimColor;
	int iHeadShotId;
	UniqueTexture hWepTex; // Added for PNG icons

	DrawBgType DrawBg;
};

#define MAX_DEATHNOTICES	4
static int DEATHNOTICE_DISPLAY_TIME = 6;
static int KILLEFFECT_DISPLAY_TIME = 4;
static int KILLICON_DISPLAY_TIME = 1;

#define DEATHNOTICE_TOP		32

DeathNoticeItem rgDeathNoticeList[ MAX_DEATHNOTICES + 1 ];

int CHudDeathNotice :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( DeathMsg );

	hud_deathnotice_time = CVAR_CREATE( "hud_deathnotice_time", "6", 0 );
	m_iFlags = 0;

	return 1;
}

static float piercingTime; 
static float PIERCINGTIME_EFFECT = 0.1;


void CHudDeathNotice::Reset(void)
{
	m_killNums = 0;
	m_multiKills = 0;
	m_showIcon = false;
	m_showKill = false;
	m_iconIndex = 0;
	m_killEffectTime = 0;
	m_killIconTime = 0;
	piercingTime = 0;
}

void CHudDeathNotice :: InitHUDData( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}


int CHudDeathNotice :: VidInit( void )
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );
	m_HUD_d_headshot = gHUD.GetSpriteIndex("d_headshot");

	m_KM_Number0 = gHUD.GetSpriteIndex("KM_Number0");
	m_KM_Number1 = gHUD.GetSpriteIndex("KM_Number1");
	m_KM_Number2 = gHUD.GetSpriteIndex("KM_Number2");
	m_KM_Number3 = gHUD.GetSpriteIndex("KM_Number3");
	m_KM_KillText = gHUD.GetSpriteIndex("KM_KillText");
	m_KM_Icon_Head = gHUD.GetSpriteIndex("KM_Icon_Head");
	m_KM_Icon_Knife = gHUD.GetSpriteIndex("KM_Icon_knife");
	m_KM_Icon_Frag = gHUD.GetSpriteIndex("KM_Icon_Frag");

	R_InitTexture(m_killBg[1], "gfx/billflx/death/ann_center.png");
	R_InitTexture(m_deathBg[1], "gfx/billflx/death/ann_center.png");
	R_InitTexture(m_ann_hs[1], "gfx/billflx/death/ann_headshot.png");

	return 1;
}

void CHudDeathNotice::Shutdown(void)
{
	std::fill(std::begin(m_killBg), std::end(m_killBg), nullptr);
	std::fill(std::begin(m_deathBg), std::end(m_deathBg), nullptr);
}

int CHudDeathNotice :: Draw( float flTime )
{
	if( m_HUD_d_skull <= 0 || m_HUD_d_headshot <= 0 )
		return 0;

	int x, y, r, g, b, i;

	for( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iId == 0 )
			break;  // we've gone through them all

		if ( rgDeathNoticeList[i].flDisplayTime < flTime )
		{ // display time has expired
			// remove the current item from the list
			memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i) );
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = min( rgDeathNoticeList[i].flDisplayTime, flTime + DEATHNOTICE_DISPLAY_TIME );

		{
			// Draw the death notice
			if( !g_iUser1 )
			{
				y = YRES(DEATHNOTICE_TOP) + 2 + (25 * i);  //!!!
			}
			else
			{
				y = ScreenHeight / 5 + 2 + (25 * i);
			}

			int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
			if( id <= 0 )
				continue;

			int iWepWidth = rgDeathNoticeList[i].hWepTex ? 50 : (gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left);
			int iVictimWidth = DrawUtils::ConsoleStringLen(rgDeathNoticeList[i].szVictim);
			int iKillerWidth = rgDeathNoticeList[i].bSuicide ? 0 : DrawUtils::ConsoleStringLen(rgDeathNoticeList[i].szKiller);
			
			x = ScreenWidth - iVictimWidth - iWepWidth - (YRES(5) * 3);
			if( rgDeathNoticeList[i].iHeadShotId )
				x -= (rgDeathNoticeList[i].hWepTex ? 20 : (gHUD.GetSpriteRect(m_HUD_d_headshot).right - gHUD.GetSpriteRect(m_HUD_d_headshot).left));

			int xMin = x;
			if (!rgDeathNoticeList[i].bSuicide)
				xMin -= (5 + iKillerWidth);

			int xOffset = 3;

			// Animation Logic: Scale for headshot only, Alpha for everyone
			float flScale = 1.0f;
			float flAlpha = 255.0f;
			float flLife = rgDeathNoticeList[i].flDisplayTime - flTime;
			float flElapsed = flTime - rgDeathNoticeList[i].flStartTime;

			if (flLife < 0.5f) {
				flAlpha = (flLife / 0.5f) * 255.0f;
			}

			gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
			gEngfuncs.pTriAPI->Color4ub(255, 255, 255, flAlpha);

			SharedTexture (*DrawBg)[3] = nullptr;
			// Only show background if WE are the killer
			if (rgDeathNoticeList[i].DrawBg == DB_KILL)
			{
				DrawBg = &m_killBg;
			}

			if (DrawBg)
			{
				(*DrawBg)[1]->Bind();
				gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
				gEngfuncs.pTriAPI->Color4ub(255, 255, 255, flAlpha);

				int bgW = ScreenWidth - xMin + 10;
				int bgH = 26;
				int bgX = xMin - 5;
				int bgY = y;

				DrawUtils::Draw2DQuadScaled(bgX, bgY, bgX + bgW, bgY + bgH);
			}

			if ( !rgDeathNoticeList[i].bSuicide )
			{
				// Draw killers name
				if ( rgDeathNoticeList[i].KillerColor )
					DrawUtils::SetConsoleTextColor( rgDeathNoticeList[i].KillerColor[0], rgDeathNoticeList[i].KillerColor[1], rgDeathNoticeList[i].KillerColor[2] );

				int killerX = x - (5 + iKillerWidth);
				DrawUtils::DrawConsoleString( killerX, y, rgDeathNoticeList[i].szKiller );
			}
			
			r = 255;  g = 255;	b = 255;
			if ( rgDeathNoticeList[i].bTeamKill )
			{
				r = 10;	g = 240; b = 10;  // display it in sickly green
			}

			// Draw death weapon
			if (rgDeathNoticeList[i].hWepTex)
			{
				rgDeathNoticeList[i].hWepTex->Bind();
				gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
				gEngfuncs.pTriAPI->Color4ub(255, 255, 255, flAlpha);
				
				int w = 50 * flScale;
				int h = 25 * flScale;
				DrawUtils::Draw2DQuadScaled(x, y, x + w, y + h);
				x += w;
			}
			else
			{
				SPR_Set( gHUD.GetSprite(id), r, g, b );
				SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(id) );
				x += iWepWidth;
			}

			if( rgDeathNoticeList[i].iHeadShotId)
			{
				// Apply scale animation only to headshot
				float flHSScale = 1.0f;
				if (flElapsed < 0.2f) {
					flHSScale = 0.5f + (flElapsed / 0.2f) * 0.7f; // Pop from 0.5 to 1.2
				} else if (flElapsed < 0.4f) {
					flHSScale = 1.2f - ((flElapsed - 0.2f) / 0.2f) * 0.2f; // Settle to 1.0
				}

				m_ann_hs[1]->Bind();
				gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
				gEngfuncs.pTriAPI->Color4ub(255, 255, 255, flAlpha);
				
				int hsW = 50 * flHSScale;
				int hsH = 50 * flHSScale;
				DrawUtils::Draw2DQuadScaled(x, y - (hsH/4), x + hsW, y + hsH - (hsH/4));
				x += hsW;
			}

			// Draw victims name (if it was a player that was killed)
			if (!rgDeathNoticeList[i].bNonPlayerKill)
			{
				if ( rgDeathNoticeList[i].VictimColor )
					DrawUtils::SetConsoleTextColor( rgDeathNoticeList[i].VictimColor[0], rgDeathNoticeList[i].VictimColor[1], rgDeathNoticeList[i].VictimColor[2] );
				DrawUtils::DrawConsoleString( x, y, rgDeathNoticeList[i].szVictim );
			}
		}
	}

	if (m_showKill)
	{
		if( m_KM_Number0 <= 0 || m_KM_Number1 <= 0 || m_KM_Number2 <= 0 || m_KM_Number3 <= 0 || m_KM_KillText <= 0 )
			return 1;

		m_killEffectTime = min(m_killEffectTime, gHUD.m_flTime + KILLEFFECT_DISPLAY_TIME);
		piercingTime = min(piercingTime, gHUD.m_flTime + PIERCINGTIME_EFFECT);

		if (gHUD.m_flTime >= m_killEffectTime)
		{
			m_showKill = false;
			m_showIcon = false;
		}
	}

	if( i == 0 )
		m_iFlags &= ~HUD_DRAW; // disable hud item

	return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice :: MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_DRAW;

	BufferReader reader( pszName, pbuf, iSize );

	bool mass_time = FALSE;

	int killer = reader.ReadByte();
	int victim = reader.ReadByte();
	int headshot = reader.ReadByte();
	int multiKills = 0;
	int idx = gEngfuncs.GetLocalPlayer()->index;

	char killedwith[32];
	strncpy( killedwith, "d_", sizeof(killedwith) );
	strcat( killedwith, reader.ReadString() );

	gHUD.m_Scoreboard.DeathMsg( killer, victim );
	gHUD.m_Spectator.DeathMessage(victim);

	for (int j = 0; j < MAX_DEATHNOTICES; j++)
	{
		if (rgDeathNoticeList[j].iId == 0)
			break;

		if (rgDeathNoticeList[j].DrawBg == DB_KILL)
			multiKills++;
	}

	if (killer == idx && victim != idx)
	{
		m_killNums++;
		m_showIcon = false;
		
		if (!strcmp(killedwith, "d_grenade") || !strcmp(killedwith, "d_m3") || !strcmp(killedwith, "d_m1887_w") || !strcmp(killedwith, "d_m1887") || !strcmp(killedwith, "d_spas_15"))
			mass_time = TRUE;
		else if (!strcmp(killedwith, "d_zombie_s"))
			mass_time = TRUE;
		else
			mass_time = FALSE;
	}

	// Logic for special kills (slugger, gunner, etc)
	if ( !strcmp(killedwith, "d_knife") || !strcmp(killedwith, "d_arabian_sword") || !strcmp(killedwith, "d_amok") || !strcmp(killedwith, "d_butterfly") || !strcmp(killedwith, "d_candy_cane") || !strcmp(killedwith, "d_combat") || !strcmp(killedwith, "d_dual_knife") || !strcmp(killedwith, "d_fangblade") || !strcmp(killedwith, "d_brass_knuckle") ||!strcmp(killedwith, "d_mini_axe") || !strcmp(killedwith, "d_ice") || !strcmp(killedwith, "d_karambit") || !strcmp(killedwith, "d_keris") || !strcmp(killedwith, "d_knifebone") || !strcmp(killedwith, "d_saber"))
	{
		if (killer == idx || victim == idx) gHUD.slugger_kill = TRUE;
	}
	else gHUD.slugger_kill = FALSE;

	if ( !strcmp(killedwith, "d_k5") || !strcmp(killedwith, "d_bow") || !strcmp(killedwith, "d_colt_python") || !strcmp(killedwith, "d_deagle") || !strcmp(killedwith, "d_deagle_dual") || !strcmp(killedwith, "d_dual_handgun") || !strcmp(killedwith, "d_taurus_raging_bull") || !strcmp(killedwith, "d_glock18") || !strcmp(killedwith, "d_usp"))
	{
		if (killer == idx || victim == idx) gHUD.special_gunner = TRUE;
	}
	else gHUD.special_gunner = FALSE;

	if (!strcmp(killedwith, "d_grenade"))
	{
		if (killer == idx || victim == idx) gHUD.bomb_shot = TRUE;
	}
	else gHUD.bomb_shot = FALSE;

	if (killer == idx && victim != idx)
	{
		if (gHUD.m_flTime < piercingTime)
		{
			if (mass_time) gHUD.mass_kill = TRUE;
			else gHUD.piercing_shot = TRUE;
		}
		else
		{
			gHUD.piercing_shot = FALSE;
			gHUD.mass_kill = FALSE;
		}
		piercingTime = gHUD.m_flTime + PIERCINGTIME_EFFECT;
		m_multiKills = multiKills + 1;
	}

	int i;
	for ( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iId == 0 )
			break;
	}
	if ( i == MAX_DEATHNOTICES )
	{
		memmove( rgDeathNoticeList, rgDeathNoticeList+1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	gHUD.m_Scoreboard.GetAllPlayersInfo();

	// Killer Name
	const char *killer_name = g_PlayerInfoList[ killer ].name;
	if ( !killer_name ) rgDeathNoticeList[i].szKiller[0] = 0;
	else {
		rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
		strncpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
	}

	// Victim Name
	const char *victim_name = NULL;
	if ( ((char)victim) != -1 ) victim_name = g_PlayerInfoList[ victim ].name;
	if ( !victim_name ) rgDeathNoticeList[i].szVictim[0] = 0;
	else {
		rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
		strncpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );
	}

	if ( ((char)victim) == -1 ) {
		rgDeathNoticeList[i].bNonPlayerKill = true;
		strncpy( rgDeathNoticeList[i].szVictim, killedwith+2, 32 );
	} else {
		if ( killer == victim || killer == 0 ) rgDeathNoticeList[i].bSuicide = true;
		if ( !strncmp( killedwith, "d_teammate", 32 ) ) rgDeathNoticeList[i].bTeamKill = true;
	}

	rgDeathNoticeList[i].iHeadShotId = headshot;
	rgDeathNoticeList[i].iId = gHUD.GetSpriteIndex( killedwith );
	
	// Load PNG Weapon Icon
	rgDeathNoticeList[i].hWepTex = nullptr;
	if (killedwith && killedwith[0] && killedwith[1] && killedwith[0] == 'd' && killedwith[1] == '_')
	{
		const char *weaponIconName = killedwith + 2;
		if (!strcmp(weaponIconName, "870mcs"))
			weaponIconName = "m3";

		const char *candidates[6] = {
			weaponIconName,
			nullptr, // _fc_bomb -> _fc
			nullptr, // _fc_bomb -> base
			nullptr, // _bomb -> base
			nullptr, // _fc -> base
			nullptr, // _cg -> base
		};

		char tmp[64];
		const char *p = weaponIconName;

		// Derive fallback candidates by stripping known suffixes.
		if (strstr(p, "_fc_bomb"))
		{
			strncpy(tmp, p, sizeof(tmp) - 1);
			tmp[sizeof(tmp) - 1] = 0;
			char *s = strstr(tmp, "_fc_bomb");
			if (s) *s = 0;
			static char fcName[64];
			snprintf(fcName, sizeof(fcName), "%s_fc", tmp);
			candidates[1] = fcName;
			candidates[2] = tmp;
		}
		else if (strstr(p, "_bomb"))
		{
			strncpy(tmp, p, sizeof(tmp) - 1);
			tmp[sizeof(tmp) - 1] = 0;
			char *s = strstr(tmp, "_bomb");
			if (s) *s = 0;
			candidates[3] = tmp;
		}
		else if (strstr(p, "_fc"))
		{
			strncpy(tmp, p, sizeof(tmp) - 1);
			tmp[sizeof(tmp) - 1] = 0;
			char *s = strstr(tmp, "_fc");
			if (s) *s = 0;
			candidates[4] = tmp;
		}
		else if (strstr(p, "_cg"))
		{
			strncpy(tmp, p, sizeof(tmp) - 1);
			tmp[sizeof(tmp) - 1] = 0;
			char *s = strstr(tmp, "_cg");
			if (s) *s = 0;
			candidates[5] = tmp;
		}

		char wepPath[256];
		for (int ci = 0; ci < (int)(sizeof(candidates) / sizeof(candidates[0])); ci++)
		{
			if (!candidates[ci] || !candidates[ci][0])
				continue;

			// Deathnotice uses small HUD icons first; fallback to the larger buy/inventory icons.
			sprintf(wepPath, "gfx/billflx/death/weapons/weapon_%s.png", candidates[ci]);
			rgDeathNoticeList[i].hWepTex = R_LoadTextureUnique(wepPath);
			if (rgDeathNoticeList[i].hWepTex)
				break;

			sprintf(wepPath, "gfx/billflx/weapons/weapon_%s.png", candidates[ci]);
			rgDeathNoticeList[i].hWepTex = R_LoadTextureUnique(wepPath);
			if (rgDeathNoticeList[i].hWepTex)
				break;
		}

		// Final fallback: generic icon
		if (!rgDeathNoticeList[i].hWepTex)
			rgDeathNoticeList[i].hWepTex = R_LoadTextureUnique("gfx/billflx/death/weapons/null.png");
	}

	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + hud_deathnotice_time->value;
	rgDeathNoticeList[i].flStartTime = gHUD.m_flTime;

	if (victim == idx) rgDeathNoticeList[i].DrawBg = DB_DEATH;
	else if (killer == idx) rgDeathNoticeList[i].DrawBg = DB_KILL;
	else rgDeathNoticeList[i].DrawBg = DB_NONE;

	return 1;
}
