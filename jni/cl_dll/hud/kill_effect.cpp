/*
kill_effect.cpp - Point Blank alerts implementation (V20 Modernized)
*/

#include "hud.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "cl_util.h"
#include "draw_util.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "parsemsg.h"
#include "eventscripts.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "event_api.h"
#include "com_weapons.h"
#include "kill_effect.h"

static bool R_LoadTextureBillflx(UniqueTexture &outTex, const char *relativeBillPath)
{
	if (!relativeBillPath || !relativeBillPath[0])
		return false;

	char path[256];

	sprintf(path, "gfx/billflx/%s", relativeBillPath);
	outTex = R_LoadTextureUnique(path);
	return static_cast<bool>(outTex);
}

// --- Helper for Frag History ---
void CHudKillEffect::AddFragToHistory(int fragIndex) {
	// Newest should appear on the left, older shift to the right.
	// Keep at most 8 entries (PB-style row).
	const int kMaxFrags = 8;

	if (m_fragCount < kMaxFrags)
		m_fragCount++;

	for (int i = m_fragCount - 1; i > 0; i--) {
		m_fragHistory[i] = m_fragHistory[i - 1];
		m_fragRowAnim[i] = m_fragRowAnim[i - 1];
	}

	m_fragHistory[0] = fragIndex;
	m_fragRowAnim[0] = 100.0f;
}

// --- Combat Macro Declarations ---
DECLARE_MESSAGE(m_KillEffect, Add_point);
DECLARE_COMMAND(m_KillEffect, CommandActiveAdd_point);
DECLARE_MESSAGE(m_KillEffect, killframe);
DECLARE_COMMAND(m_KillEffect, CommandActivekillframe);
DECLARE_MESSAGE(m_KillEffect, killframeAnim);
DECLARE_COMMAND(m_KillEffect, CommandActivekillframeAnim);
DECLARE_MESSAGE(m_KillEffect, MissionComplete);
DECLARE_COMMAND(m_KillEffect, CommandActiveMissionComplete);
DECLARE_MESSAGE(m_KillEffect, Pointkill);
DECLARE_COMMAND(m_KillEffect, CommandActivePointkill);
DECLARE_MESSAGE(m_KillEffect, PiercingShot);
DECLARE_COMMAND(m_KillEffect, CommandActivePiercingShot);
DECLARE_MESSAGE(m_KillEffect, MassKill);
DECLARE_COMMAND(m_KillEffect, CommandActiveMassKill);
DECLARE_MESSAGE(m_KillEffect, Doublekill);
DECLARE_COMMAND(m_KillEffect, CommandActiveDoublekill);
DECLARE_MESSAGE(m_KillEffect, Triplekill);
DECLARE_COMMAND(m_KillEffect, CommandActiveTriplekill);
DECLARE_MESSAGE(m_KillEffect, Chainkiller);
DECLARE_COMMAND(m_KillEffect, CommandActiveChainkiller);
DECLARE_MESSAGE(m_KillEffect, HeadshotPoint);
DECLARE_COMMAND(m_KillEffect, CommandActiveHeadshotPoint);
DECLARE_MESSAGE(m_KillEffect, Headshot);
DECLARE_COMMAND(m_KillEffect, CommandActiveHeadshot);
DECLARE_MESSAGE(m_KillEffect, ChainHeadshot);
DECLARE_COMMAND(m_KillEffect, CommandActiveChainHeadshot);
DECLARE_MESSAGE(m_KillEffect, Helmet);
DECLARE_COMMAND(m_KillEffect, CommandActiveHelmet);
DECLARE_MESSAGE(m_KillEffect, Stopper);
DECLARE_COMMAND(m_KillEffect, CommandActiveStopper);
DECLARE_MESSAGE(m_KillEffect, Slugger);
DECLARE_COMMAND(m_KillEffect, CommandActiveSlugger);
DECLARE_MESSAGE(m_KillEffect, PointNumber);
DECLARE_COMMAND(m_KillEffect, CommandActivePointNumber);
DECLARE_MESSAGE(m_KillEffect, HitMarker);
DECLARE_COMMAND(m_KillEffect, CommandActiveHitMarker);
DECLARE_MESSAGE(m_KillEffect, HotKiller);
DECLARE_COMMAND(m_KillEffect, CommandActiveHotKiller);
DECLARE_MESSAGE(m_KillEffect, Nightmare);
DECLARE_COMMAND(m_KillEffect, CommandActiveNightmare);
DECLARE_COMMAND(m_KillEffect, CommandActiveassist);
DECLARE_MESSAGE(m_KillEffect, FragAnimKill);
DECLARE_COMMAND(m_KillEffect, CommandActiveFragAnimKill);
DECLARE_MESSAGE(m_KillEffect, FragAnimHs);
DECLARE_COMMAND(m_KillEffect, CommandActiveFragAnimHs);
DECLARE_MESSAGE(m_KillEffect, FragAnimStopper);
DECLARE_COMMAND(m_KillEffect, CommandActiveFragAnimStopper);
DECLARE_MESSAGE(m_KillEffect, FragAnimStopperHs);
DECLARE_COMMAND(m_KillEffect, CommandActiveFragAnimStopperHs);
DECLARE_MESSAGE(m_KillEffect, FragAnimBlue);
DECLARE_COMMAND(m_KillEffect, CommandActiveFragAnimBlue);
DECLARE_MESSAGE(m_KillEffect, FragAnimGold);
DECLARE_COMMAND(m_KillEffect, CommandActiveFragAnimGold);
DECLARE_MESSAGE(m_KillEffect, SpecialGunner);
DECLARE_COMMAND(m_KillEffect, CommandActiveSpecialGunner);
DECLARE_MESSAGE(m_KillEffect, BombShot);
DECLARE_COMMAND(m_KillEffect, CommandActiveBombShot);
DECLARE_MESSAGE(m_KillEffect, oneShot);
DECLARE_COMMAND(m_KillEffect, CommandActiveoneShot);
DECLARE_MESSAGE(m_KillEffect, OneshotEnable);
DECLARE_COMMAND(m_KillEffect, CommandActiveOneshotEnable);
DECLARE_MESSAGE(m_KillEffect, OneshotDisable);
DECLARE_COMMAND(m_KillEffect, CommandActiveOneshotDisable);

// --- Shop Macro Declarations ---
DECLARE_MESSAGE(m_KillEffect, buy_qc);
DECLARE_COMMAND(m_KillEffect, CommandActivebuy_qc);
DECLARE_MESSAGE(m_KillEffect, buy_megahp);
DECLARE_COMMAND(m_KillEffect, CommandActivebuy_megahp);
DECLARE_MESSAGE(m_KillEffect, buy_bpoint);
DECLARE_COMMAND(m_KillEffect, CommandActivebuy_bpoint);
DECLARE_MESSAGE(m_KillEffect, buy_qr);
DECLARE_COMMAND(m_KillEffect, CommandActivebuy_qr);
DECLARE_MESSAGE(m_KillEffect, buy_qrespawn);
DECLARE_COMMAND(m_KillEffect, CommandActivebuy_qrespawn);
DECLARE_MESSAGE(m_KillEffect, Unequip_mask);
DECLARE_COMMAND(m_KillEffect, CommandActiveUnequip_mask);
DECLARE_MESSAGE(m_KillEffect, Count_unit);
DECLARE_COMMAND(m_KillEffect, CommandActiveCount_unit);

DECLARE_MESSAGE(m_KillEffect, buy_mask_1); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_1);
DECLARE_MESSAGE(m_KillEffect, buy_mask_2); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_2);
DECLARE_MESSAGE(m_KillEffect, buy_mask_3); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_3);
DECLARE_MESSAGE(m_KillEffect, buy_mask_4); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_4);
DECLARE_MESSAGE(m_KillEffect, buy_mask_5); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_5);
DECLARE_MESSAGE(m_KillEffect, buy_mask_6); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_6);
DECLARE_MESSAGE(m_KillEffect, buy_mask_7); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_7);
DECLARE_MESSAGE(m_KillEffect, buy_mask_8); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_8);
DECLARE_MESSAGE(m_KillEffect, buy_mask_9); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_9);
DECLARE_MESSAGE(m_KillEffect, buy_mask_10); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_10);
DECLARE_MESSAGE(m_KillEffect, buy_mask_11); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_11);
DECLARE_MESSAGE(m_KillEffect, buy_mask_12); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_12);
DECLARE_MESSAGE(m_KillEffect, buy_mask_13); DECLARE_COMMAND(m_KillEffect, CommandActivebuy_mask_13);

// --- Combat Logic & Animation States ---
bool isPiercingShot, isMassKill, isDoublekill, isTriplekill, isChainkiller;
bool isHeadshot, isChainHeadshot, isHelmet, isStopper, isSlugger;
bool isHotKiller, isNightmare, isSpecialGunner, isBombShot;

float dis_PiercingShot, dis_MassKill, dis_Doublekill, dis_Triplekill, dis_Chainkiller;
float dis_Headshot, dis_ChainHeadshot, dis_Helmet, dis_Stopper, dis_Slugger;
float dis_HotKiller, dis_Nightmare, dis_SpecialGunner, dis_BombShot;

float KillDisplay() {
    if (isPiercingShot) dis_PiercingShot = 100.0f; else dis_PiercingShot = 0.0f;
    if (isMassKill) dis_MassKill = 100.0f; else dis_MassKill = 0.0f;
    if (isDoublekill) dis_Doublekill = 100.0f; else dis_Doublekill = 0.0f;
    if (isTriplekill) dis_Triplekill = 100.0f; else dis_Triplekill = 0.0f;
    if (isChainkiller) dis_Chainkiller = 100.0f; else dis_Chainkiller = 0.0f;
    if (isHeadshot) dis_Headshot = 100.0f; else dis_Headshot = 0.0f;
    if (isChainHeadshot) dis_ChainHeadshot = 100.0f; else dis_ChainHeadshot = 0.0f;
    if (isHelmet) dis_Helmet = 100.0f; else dis_Helmet = 0.0f;
    if (isStopper) dis_Stopper = 100.0f; else dis_Stopper = 0.0f;
    if (isSlugger) dis_Slugger = 100.0f; else dis_Slugger = 0.0f;
    if (isHotKiller) dis_HotKiller = 100.0f; else dis_HotKiller = 0.0f;
    if (isNightmare) dis_Nightmare = 100.0f; else dis_Nightmare = 0.0f;
    if (isSpecialGunner) dis_SpecialGunner = 100.0f; else dis_SpecialGunner = 0.0f;
    if (isBombShot) dis_BombShot = 100.0f; else dis_BombShot = 0.0f;
    
    return dis_PiercingShot + dis_MassKill + dis_Doublekill + dis_Triplekill + dis_Chainkiller + 
           dis_Headshot + dis_ChainHeadshot + dis_Helmet + dis_Stopper + dis_Slugger + 
           dis_HotKiller + dis_Nightmare + dis_SpecialGunner + dis_BombShot;
}

void CHudKillEffect::Reset(void) {
    isPiercingShot=isMassKill=isDoublekill=isTriplekill=isChainkiller=FALSE;
    isHeadshot=isChainHeadshot=isHelmet=isStopper=isSlugger=FALSE;
    isHotKiller=isNightmare=isSpecialGunner=isBombShot=FALSE;
    current_blood_frame = 0; last_frag_id = -1; is_blood_anim_active = false;
    m_center_anim_start_time = 0.0f;
    m_pending_frag_id = -1;
    m_pending_frag_added = false;
}

// --- Combat Command Handlers ---
static void StartCenterAnim(CHudKillEffect *self, int fragId)
{
    if (!self)
        return;

    self->is_blood_anim_active = true;
    self->current_blood_frame = 0;
    self->m_center_anim_start_time = (float)gEngfuncs.GetClientTime();
    self->last_frag_id = fragId;
    self->m_pending_frag_id = fragId;
    self->m_pending_frag_added = false;
}

void CHudKillEffect::UserCmd_CommandActivePointkill(void) {
    Reset();
    if(gHUD.slugger_kill) { StartCenterAnim(this, 5); }
    else { StartCenterAnim(this, 0); }
}
void CHudKillEffect::UserCmd_CommandActiveHeadshotPoint(void) {
    Reset();
    StartCenterAnim(this, gHUD.slugger_kill ? 9 : 1);
}
void CHudKillEffect::UserCmd_CommandActiveDoublekill(void) {
    Reset();
    isDoublekill=TRUE; Doublekill_time = (long)KillDisplay(); ClientCmd("spk vox/doublekill.wav");
    StartCenterAnim(this, 0);
}
void CHudKillEffect::UserCmd_CommandActiveTriplekill(void) {
    Reset();
    isTriplekill=TRUE; Triplekill_time = (long)KillDisplay(); ClientCmd("spk vox/triplekill.wav");
    StartCenterAnim(this, 0);
}
void CHudKillEffect::UserCmd_CommandActiveChainkiller(void) {
    Reset();
    isChainkiller=TRUE; Chainkiller_time = (long)KillDisplay(); ClientCmd("spk vox/chainkiller.wav");
    StartCenterAnim(this, 0);
}
void CHudKillEffect::UserCmd_CommandActiveHeadshot(void) {
    Reset();
    isHeadshot=TRUE; Headshot_time = (long)KillDisplay(); ClientCmd("spk vox/headshot.wav");
    StartCenterAnim(this, gHUD.slugger_kill ? 9 : 1);
}
void CHudKillEffect::UserCmd_CommandActiveChainHeadshot(void) {
    Reset();
    isChainHeadshot=TRUE; ChainHeadshot_time = (long)KillDisplay(); ClientCmd("spk vox/chainheadshot.wav");
    StartCenterAnim(this, 1);
}
void CHudKillEffect::UserCmd_CommandActiveStopper(void) {
    Reset();
    isStopper=TRUE; Stopper_time = (long)KillDisplay(); ClientCmd("spk vox/stopper.wav");
    StartCenterAnim(this, gHUD.slugger_kill ? 10 : 2);
}

void CHudKillEffect::UserCmd_CommandActiveMissionComplete(void) { MissionComplete_time = 250; }
void CHudKillEffect::UserCmd_CommandActivekillframe(void) { killframe_time = 40; }
void CHudKillEffect::UserCmd_CommandActivekillframeAnim(void) { killframeAnim_time = 35; }
void CHudKillEffect::UserCmd_CommandActiveOneshotEnable(void) {}
void CHudKillEffect::UserCmd_CommandActiveOneshotDisable(void) {}
void CHudKillEffect::UserCmd_CommandActivePiercingShot(void) {
    Reset();
    isPiercingShot = TRUE;
    PiercingShot_time = (long)KillDisplay();
    StartCenterAnim(this, 0);
}
void CHudKillEffect::UserCmd_CommandActiveMassKill(void) {
    Reset();
    isMassKill = TRUE;
    MassKill_time = (long)KillDisplay();
    StartCenterAnim(this, 6);
}
void CHudKillEffect::UserCmd_CommandActiveSlugger(void) {
    Reset();
    isSlugger = TRUE;
    Slugger_time = (long)KillDisplay();
    StartCenterAnim(this, 5);
}
void CHudKillEffect::UserCmd_CommandActivePointNumber(void) {}
void CHudKillEffect::UserCmd_CommandActiveHitMarker(void) {}
void CHudKillEffect::UserCmd_CommandActiveHotKiller(void) {
    isHotKiller = TRUE;
    HotKiller_time = (long)KillDisplay();
    if (!is_blood_anim_active)
        StartCenterAnim(this, 4);
}
void CHudKillEffect::UserCmd_CommandActiveNightmare(void) {
    isNightmare = TRUE;
    Nightmare_time = (long)KillDisplay();
    if (!is_blood_anim_active)
        StartCenterAnim(this, 3);
}
void CHudKillEffect::UserCmd_CommandActiveassist(void) {
    assist_time = 80;
}
void CHudKillEffect::UserCmd_CommandActiveFragAnimKill(void) { if (!is_blood_anim_active) StartCenterAnim(this, 0); else { last_frag_id = 0; m_pending_frag_id = 0; m_pending_frag_added = false; } }
void CHudKillEffect::UserCmd_CommandActiveFragAnimHs(void) { if (!is_blood_anim_active) StartCenterAnim(this, 1); else { last_frag_id = 1; m_pending_frag_id = 1; m_pending_frag_added = false; } }
void CHudKillEffect::UserCmd_CommandActiveFragAnimStopper(void) { if (!is_blood_anim_active) StartCenterAnim(this, 2); else { last_frag_id = 2; m_pending_frag_id = 2; m_pending_frag_added = false; } }
void CHudKillEffect::UserCmd_CommandActiveFragAnimStopperHs(void) { if (!is_blood_anim_active) StartCenterAnim(this, 7); else { last_frag_id = 7; m_pending_frag_id = 7; m_pending_frag_added = false; } }
void CHudKillEffect::UserCmd_CommandActiveFragAnimBlue(void) { if (!is_blood_anim_active) StartCenterAnim(this, 3); else { last_frag_id = 3; m_pending_frag_id = 3; m_pending_frag_added = false; } }
void CHudKillEffect::UserCmd_CommandActiveFragAnimGold(void) { if (!is_blood_anim_active) StartCenterAnim(this, 4); else { last_frag_id = 4; m_pending_frag_id = 4; m_pending_frag_added = false; } }
void CHudKillEffect::UserCmd_CommandActiveSpecialGunner(void) {
    Reset();
    isSpecialGunner = TRUE;
    SpecialGunner_time = (long)KillDisplay();
    StartCenterAnim(this, 0);
}
void CHudKillEffect::UserCmd_CommandActiveBombShot(void) {
    Reset();
    isBombShot = TRUE;
    BombShot_time = (long)KillDisplay();
    StartCenterAnim(this, 6);
}
void CHudKillEffect::UserCmd_CommandActiveoneShot(void) {
    Reset();
    oneShot_time = (long)KillDisplay();
    StartCenterAnim(this, 0);
}
void CHudKillEffect::UserCmd_CommandActiveHelmet(void) {
    Reset();
    isHelmet = TRUE;
    Helmet_time = (long)KillDisplay();
}

// --- MsgFunc Hooks ---
int CHudKillEffect::MsgFunc_Pointkill(const char *pszName, int iSize, void *pbuf) { ClientCmd("Pointkill"); return 1; }
int CHudKillEffect::MsgFunc_HeadshotPoint(const char *pszName, int iSize, void *pbuf) { ClientCmd("HeadshotPoint"); return 1; }
int CHudKillEffect::MsgFunc_Doublekill(const char *pszName, int iSize, void *pbuf) { ClientCmd("Doublekill"); return 1; }
int CHudKillEffect::MsgFunc_Triplekill(const char *pszName, int iSize, void *pbuf) { ClientCmd("Triplekill"); return 1; }
int CHudKillEffect::MsgFunc_Chainkiller(const char *pszName, int iSize, void *pbuf) { ClientCmd("Chainkiller"); return 1; }
int CHudKillEffect::MsgFunc_Headshot(const char *pszName, int iSize, void *pbuf) { ClientCmd("Headshot"); return 1; }
int CHudKillEffect::MsgFunc_ChainHeadshot(const char *pszName, int iSize, void *pbuf) { ClientCmd("ChainHeadshot"); return 1; }
int CHudKillEffect::MsgFunc_Stopper(const char *pszName, int iSize, void *pbuf) { ClientCmd("Stopper"); return 1; }
int CHudKillEffect::MsgFunc_Helmet(const char *pszName, int iSize, void *pbuf) { ClientCmd("Helmet"); return 1; }
int CHudKillEffect::MsgFunc_Slugger(const char *pszName, int iSize, void *pbuf) { ClientCmd("Slugger"); return 1; }
int CHudKillEffect::MsgFunc_Add_point(const char *pszName, int iSize, void *pbuf) { ClientCmd("Add_point"); return 1; }
int CHudKillEffect::MsgFunc_killframe(const char *pszName, int iSize, void *pbuf) { ClientCmd("killframe"); return 1; }
int CHudKillEffect::MsgFunc_killframeAnim(const char *pszName, int iSize, void *pbuf) { ClientCmd("killframeAnim"); return 1; }
int CHudKillEffect::MsgFunc_MissionComplete(const char *pszName, int iSize, void *pbuf) { ClientCmd("MissionComplete"); return 1; }
int CHudKillEffect::MsgFunc_PiercingShot(const char *pszName, int iSize, void *pbuf) { ClientCmd("PiercingShot"); return 1; }
int CHudKillEffect::MsgFunc_MassKill(const char *pszName, int iSize, void *pbuf) { ClientCmd("MassKill"); return 1; }
int CHudKillEffect::MsgFunc_OneshotEnable(const char *pszName, int iSize, void *pbuf) { ClientCmd("OneshotEnable"); return 1; }
int CHudKillEffect::MsgFunc_OneshotDisable(const char *pszName, int iSize, void *pbuf) { ClientCmd("OneshotDisable"); return 1; }
int CHudKillEffect::MsgFunc_PointNumber(const char *pszName, int iSize, void *pbuf) { ClientCmd("PointNumber"); return 1; }
int CHudKillEffect::MsgFunc_HitMarker(const char *pszName, int iSize, void *pbuf) { ClientCmd("HitMarker"); return 1; }
int CHudKillEffect::MsgFunc_HotKiller(const char *pszName, int iSize, void *pbuf) { ClientCmd("HotKiller"); return 1; }
int CHudKillEffect::MsgFunc_Nightmare(const char *pszName, int iSize, void *pbuf) { ClientCmd("Nightmare"); return 1; }
int CHudKillEffect::MsgFunc_SpecialGunner(const char *pszName, int iSize, void *pbuf) { ClientCmd("SpecialGunner"); return 1; }
int CHudKillEffect::MsgFunc_BombShot(const char *pszName, int iSize, void *pbuf) { ClientCmd("BombShot"); return 1; }
int CHudKillEffect::MsgFunc_oneShot(const char *pszName, int iSize, void *pbuf) { ClientCmd("oneShot"); return 1; }
int CHudKillEffect::MsgFunc_FragAnimKill(const char *pszName, int iSize, void *pbuf) { ClientCmd("FragAnimKill"); return 1; }
int CHudKillEffect::MsgFunc_FragAnimHs(const char *pszName, int iSize, void *pbuf) { ClientCmd("FragAnimHs"); return 1; }
int CHudKillEffect::MsgFunc_FragAnimStopper(const char *pszName, int iSize, void *pbuf) { ClientCmd("FragAnimStopper"); return 1; }
int CHudKillEffect::MsgFunc_FragAnimStopperHs(const char *pszName, int iSize, void *pbuf) { ClientCmd("FragAnimStopperHs"); return 1; }
int CHudKillEffect::MsgFunc_FragAnimBlue(const char *pszName, int iSize, void *pbuf) { ClientCmd("FragAnimBlue"); return 1; }
int CHudKillEffect::MsgFunc_FragAnimGold(const char *pszName, int iSize, void *pbuf) { ClientCmd("FragAnimGold"); return 1; }

// --- Global CVars ---
cvar_t *mission_kill, *mission_doublekill, *mission_triplekill, *mission_chainkiller;
cvar_t *mission_headshot, *mission_chainheadshot, *mission_slugger, *mission_masskill, *mission_piercingshot, *pb_point;

// --- Class Initialization ---
int CHudKillEffect::Init() {
    HOOK_MESSAGE(Add_point); HOOK_COMMAND("Add_point", CommandActiveAdd_point);
    HOOK_MESSAGE(Count_unit); HOOK_COMMAND("Count_unit", CommandActiveCount_unit);
    HOOK_MESSAGE(Unequip_mask); HOOK_COMMAND("Unequip_mask", CommandActiveUnequip_mask);
    HOOK_MESSAGE(buy_qc); HOOK_COMMAND("buy_qc", CommandActivebuy_qc);
    HOOK_MESSAGE(buy_megahp); HOOK_COMMAND("buy_megahp", CommandActivebuy_megahp);
    HOOK_MESSAGE(buy_bpoint); HOOK_COMMAND("buy_bpoint", CommandActivebuy_bpoint);
    HOOK_MESSAGE(buy_qr); HOOK_COMMAND("buy_qr", CommandActivebuy_qr);
    HOOK_MESSAGE(buy_qrespawn); HOOK_COMMAND("buy_qrespawn", CommandActivebuy_qrespawn);
    
    HOOK_MESSAGE(buy_mask_1); HOOK_COMMAND("buy_mask_1", CommandActivebuy_mask_1);
    HOOK_MESSAGE(buy_mask_2); HOOK_COMMAND("buy_mask_2", CommandActivebuy_mask_2);
    HOOK_MESSAGE(buy_mask_3); HOOK_COMMAND("buy_mask_3", CommandActivebuy_mask_3);
    HOOK_MESSAGE(buy_mask_4); HOOK_COMMAND("buy_mask_4", CommandActivebuy_mask_4);
    HOOK_MESSAGE(buy_mask_5); HOOK_COMMAND("buy_mask_5", CommandActivebuy_mask_5);
    HOOK_MESSAGE(buy_mask_6); HOOK_COMMAND("buy_mask_6", CommandActivebuy_mask_6);
    HOOK_MESSAGE(buy_mask_7); HOOK_COMMAND("buy_mask_7", CommandActivebuy_mask_7);
    HOOK_MESSAGE(buy_mask_8); HOOK_COMMAND("buy_mask_8", CommandActivebuy_mask_8);
    HOOK_MESSAGE(buy_mask_9); HOOK_COMMAND("buy_mask_9", CommandActivebuy_mask_9);
    HOOK_MESSAGE(buy_mask_10); HOOK_COMMAND("buy_mask_10", CommandActivebuy_mask_10);
    HOOK_MESSAGE(buy_mask_11); HOOK_COMMAND("buy_mask_11", CommandActivebuy_mask_11);
    HOOK_MESSAGE(buy_mask_12); HOOK_COMMAND("buy_mask_12", CommandActivebuy_mask_12);
    HOOK_MESSAGE(buy_mask_13); HOOK_COMMAND("buy_mask_13", CommandActivebuy_mask_13);

    HOOK_MESSAGE(killframe); HOOK_COMMAND("killframe", CommandActivekillframe);
    HOOK_MESSAGE(killframeAnim); HOOK_COMMAND("killframeAnim", CommandActivekillframeAnim);
    HOOK_MESSAGE(MissionComplete); HOOK_COMMAND("MissionComplete", CommandActiveMissionComplete);
    HOOK_MESSAGE(Pointkill); HOOK_COMMAND("Pointkill", CommandActivePointkill);
    HOOK_MESSAGE(PiercingShot); HOOK_COMMAND("PiercingShot", CommandActivePiercingShot);
    HOOK_MESSAGE(MassKill); HOOK_COMMAND("MassKill", CommandActiveMassKill);
    HOOK_MESSAGE(Doublekill); HOOK_COMMAND("Doublekill", CommandActiveDoublekill);
    HOOK_MESSAGE(Triplekill); HOOK_COMMAND("Triplekill", CommandActiveTriplekill);
    HOOK_MESSAGE(Chainkiller); HOOK_COMMAND("Chainkiller", CommandActiveChainkiller);
    HOOK_MESSAGE(HeadshotPoint); HOOK_COMMAND("HeadshotPoint", CommandActiveHeadshotPoint);
    HOOK_MESSAGE(Headshot); HOOK_COMMAND("Headshot", CommandActiveHeadshot);
    HOOK_MESSAGE(ChainHeadshot); HOOK_COMMAND("ChainHeadshot", CommandActiveChainHeadshot);
    HOOK_MESSAGE(Helmet); HOOK_COMMAND("Helmet", CommandActiveHelmet);
    HOOK_MESSAGE(Stopper); HOOK_COMMAND("Stopper", CommandActiveStopper);
    HOOK_MESSAGE(Slugger); HOOK_COMMAND("Slugger", CommandActiveSlugger);
    HOOK_MESSAGE(PointNumber); HOOK_COMMAND("PointNumber", CommandActivePointNumber);
    HOOK_MESSAGE(HitMarker); HOOK_COMMAND("HitMarker", CommandActiveHitMarker);
    HOOK_MESSAGE(HotKiller); HOOK_COMMAND("HotKiller", CommandActiveHotKiller);
    HOOK_MESSAGE(Nightmare); HOOK_COMMAND("Nightmare", CommandActiveNightmare);
    HOOK_COMMAND("assist", CommandActiveassist);
    HOOK_MESSAGE(FragAnimKill); HOOK_COMMAND("FragAnimKill", CommandActiveFragAnimKill);
    HOOK_MESSAGE(FragAnimHs); HOOK_COMMAND("FragAnimHs", CommandActiveFragAnimHs);
    HOOK_MESSAGE(FragAnimStopper); HOOK_COMMAND("FragAnimStopper", CommandActiveFragAnimStopper);
    HOOK_MESSAGE(FragAnimStopperHs); HOOK_COMMAND("FragAnimStopperHs", CommandActiveFragAnimStopperHs);
    HOOK_MESSAGE(FragAnimBlue); HOOK_COMMAND("FragAnimBlue", CommandActiveFragAnimBlue);
    HOOK_MESSAGE(FragAnimGold); HOOK_COMMAND("FragAnimGold", CommandActiveFragAnimGold);
    HOOK_MESSAGE(SpecialGunner); HOOK_COMMAND("SpecialGunner", CommandActiveSpecialGunner);
    HOOK_MESSAGE(BombShot); HOOK_COMMAND("BombShot", CommandActiveBombShot);
    HOOK_MESSAGE(oneShot); HOOK_COMMAND("oneShot", CommandActiveoneShot);
    HOOK_MESSAGE(OneshotEnable); HOOK_COMMAND("OneshotEnable", CommandActiveOneshotEnable);
    HOOK_MESSAGE(OneshotDisable); HOOK_COMMAND("OneshotDisable", CommandActiveOneshotDisable);

    gHUD.AddHudElem(this);
    return 1;
}

int CHudKillEffect::VidInit() {
    R_LoadTextureBillflx(m_killframe, "frame/frame.png");
    R_LoadTextureBillflx(m_fraganim[0], "fraganim/Frag_Kill.png");
    R_LoadTextureBillflx(m_fraganim[1], "fraganim/Frag_Headshot.png");
    R_LoadTextureBillflx(m_fraganim[2], "fraganim/Frag_Stopper.png");
    R_LoadTextureBillflx(m_fraganim[3], "fraganim/Frag_Blue_1.png");
    R_LoadTextureBillflx(m_fraganim[4], "fraganim/Frag_Gold_1.png");
    R_LoadTextureBillflx(m_fraganim[5], "fraganim/Frag_Melee.png");
    R_LoadTextureBillflx(m_fraganim[6], "fraganim/Frag_Masskill2.png");
    R_LoadTextureBillflx(m_fraganim[7], "fraganim/Frag_StopperHS.png");
    R_LoadTextureBillflx(m_fraganim[8], "fraganim/Frag_Silver_1.png");
    R_LoadTextureBillflx(m_fraganim[9], "fraganim/Frag_MeleeHS.png");
    R_LoadTextureBillflx(m_fraganim[10], "fraganim/Frag_StopperMelee.png");
    R_LoadTextureBillflx(m_fraganim[11], "fraganim/Frag_StopperMeleeHS.png");
    
    R_LoadTextureBillflx(m_kill2[0], "announcement/double.png");
    R_LoadTextureBillflx(m_kill3[0], "announcement/triple.png");
    R_LoadTextureBillflx(m_kill4[0], "announcement/chkill.png");
    R_LoadTextureBillflx(m_headshot[0], "announcement/hs.png");
    R_LoadTextureBillflx(m_chheadshot[0], "announcement/chhs.png");
    R_LoadTextureBillflx(m_chslugger[0], "announcement/chslug.png");
    R_LoadTextureBillflx(m_stopper[0], "announcement/chstop.png");
    R_LoadTextureBillflx(m_piercing[0], "announcement/piercing.png");
    R_LoadTextureBillflx(m_mass[0], "announcement/masskill.png");
    R_LoadTextureBillflx(m_hotkiller[0], "announcement/hotkill.png");
    R_LoadTextureBillflx(m_nightmare[0], "announcement/night.png");
    R_LoadTextureBillflx(m_special[0], "announcement/sg.png");
    R_LoadTextureBillflx(m_special[1], "announcement/bs.png");
    R_LoadTextureBillflx(m_special[2], "announcement/oneshot.png");
    R_LoadTextureBillflx(m_helmet[0], "announcement/helmet.png");
    R_LoadTextureBillflx(m_assist[0], "announcement/assist.png");
    
    mission_kill = CVAR_CREATE("billflxcrypted_mission_kill", "0", 0);
    pb_point = CVAR_CREATE("billflxencrypted_pb_points", "0", 0);
    
    m_fragCount = 0;
    return 1;
}

void CHudKillEffect::DrawFragRow() {
    int startX = ScreenWidth / 2 - 200;
    int finalY = ScreenHeight - 50;
    for (int i = 0; i < m_fragCount; i++) {
        int posX = startX + (i * 45);
        if (!m_fraganim[m_fragHistory[i]])
            continue;
        m_fraganim[m_fragHistory[i]]->Bind();
        gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
        DrawUtils::Draw2DQuadScaled(posX, finalY, posX + 40, finalY + 40);
    }
}

int CHudKillEffect::Draw(float flTime) {
    DrawFragRow();
    if (is_blood_anim_active) {
        int centerX = ScreenWidth / 2, centerY = ScreenHeight / 2 + 50;

        const float frameInterval = 0.07f; // ~1.0s total for 15 stages
        const int maxStage = 15;
        int stage = 0;
        if (m_center_anim_start_time > 0.0f)
            stage = (int)((flTime - m_center_anim_start_time) / frameInterval);
        if (stage < 0) stage = 0;
        if (stage > maxStage) stage = maxStage;
        current_blood_frame = stage;

        if (stage < maxStage) {
            if (m_killframe)
                m_killframe->Bind();
            gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
            DrawUtils::Draw2DQuadScaled(centerX - 110, centerY - 110, centerX + 110, centerY + 110);

            if (stage >= 4 && last_frag_id >= 0 && m_fraganim[last_frag_id]) {
                float scale = 1.0f;
                if (stage <= 8) {
                    scale = 0.6f + (stage - 4) * 0.1f;
                    if (scale > 1.0f) scale = 1.0f;
                } else if (stage >= 12) {
                    scale = 1.0f - (stage - 12) * 0.12f;
                    if (scale < 0.4f) scale = 0.4f;
                }

                float alphaF = 255.0f;
                float yDrop = 0.0f;
                if (stage >= 12) {
                    alphaF = 255.0f - (stage - 12) * 80.0f;
                    if (alphaF < 0.0f) alphaF = 0.0f;
                    yDrop = (stage - 12) * 35.0f;
                }

                if (!m_pending_frag_added && m_pending_frag_id >= 0 && stage >= 12) {
                    AddFragToHistory(m_pending_frag_id);
                    m_pending_frag_added = true;
                }

                m_fraganim[last_frag_id]->Bind();
                gEngfuncs.pTriAPI->Color4ub(255, 255, 255, (int)alphaF);
                int fw = (int)(128 * scale), fh = (int)(128 * scale);
                DrawUtils::Draw2DQuadScaled(centerX - fw/2, (int)(centerY - fh/2 + yDrop),
                                            centerX + fw/2, (int)(centerY + fh/2 + yDrop));
            }
        } else {
            is_blood_anim_active = false;
            last_frag_id = -1;
            m_pending_frag_id = -1;
            m_pending_frag_added = false;
        }
    }
    
    UniqueTexture *announcement = NULL; long *timer = NULL;
    if (HotKiller_time > 0) { announcement = m_hotkiller; timer = &HotKiller_time; }
    else if (Nightmare_time > 0) { announcement = m_nightmare; timer = &Nightmare_time; }
    else if (assist_time > 0) { announcement = m_assist; timer = &assist_time; }
    else if (oneShot_time > 0) { announcement = &m_special[2]; timer = &oneShot_time; }
    else if (BombShot_time > 0) { announcement = &m_special[1]; timer = &BombShot_time; }
    else if (SpecialGunner_time > 0) { announcement = &m_special[0]; timer = &SpecialGunner_time; }
    else if (PiercingShot_time > 0) { announcement = m_piercing; timer = &PiercingShot_time; }
    else if (MassKill_time > 0) { announcement = m_mass; timer = &MassKill_time; }
    else if (Chainkiller_time > 0) { announcement = m_kill4; timer = &Chainkiller_time; }
    else if (Triplekill_time > 0) { announcement = m_kill3; timer = &Triplekill_time; }
    else if (Doublekill_time > 0) { announcement = m_kill2; timer = &Doublekill_time; }
    else if (ChainHeadshot_time > 0) { announcement = m_chheadshot; timer = &ChainHeadshot_time; }
    else if (Slugger_time > 0) { announcement = m_chslugger; timer = &Slugger_time; }
    else if (Stopper_time > 0) { announcement = m_stopper; timer = &Stopper_time; }
    else if (Helmet_time > 0) { announcement = m_helmet; timer = &Helmet_time; }
    else if (Headshot_time > 0) { announcement = m_headshot; timer = &Headshot_time; }
    
    if (announcement && timer && *timer > 0) {
        int cx = ScreenWidth / 2, cy = ScreenHeight / 2 - 100;
        float alpha = (*timer < 20) ? (*timer / 20.0f) * 255 : 255;
        announcement[0]->Bind();
        gEngfuncs.pTriAPI->Color4ub(255, 255, 255, (int)alpha);
        DrawUtils::Draw2DQuadScaled(cx - 150, cy - 50, cx + 150, cy + 50);
        *timer -= 1;
    }
    return 1;
}

// --- SHOP FUNCTIONS ---
void CHudKillEffect::UserCmd_CommandActiveUnequip_mask(void)
{
ClientCmd("billflxcrypted_item_mask 0");
}

void CHudKillEffect::UserCmd_CommandActiveCount_unit(void)
{
/*if (gHUD.item_QuickDeploy->value == 1)
{
gHUD.unit_item_QuickDeploy->value -= 1;
gEngfuncs.Cvar_SetValue( "billflxcrypted_unit_quickdeploy", gHUD.unit_item_QuickDeploy->value);
}
if (gHUD.item_QuickReload->value == 1)
{
gHUD.unit_item_QuickReload->value -= 1;
gEngfuncs.Cvar_SetValue( "billflxcrypted_unit_quickreload", gHUD.unit_item_QuickReload->value);
}*/
}

void CHudKillEffect::UserCmd_CommandActiveAdd_point(void)
{
pb_point->value += 300;
gEngfuncs.Cvar_SetValue( "billflxencrypted_pb_points", pb_point->value);
}

void CHudKillEffect::UserCmd_CommandActivebuy_qrespawn(void)
{
ClientCmd("billflxcrypted_item_qrespawn 1");
ClientCmd("exec touch/notice; touch_addbutton \"notice_bg2\" \"#SUCCESS\" \"\" 0.360000 0.555870 0.660000 0.684148 255 255 255 214 4; play media/PB_15ver_Equip_on.wav");
}

void CHudKillEffect::UserCmd_CommandActivebuy_qc(void)
{
ClientCmd("billflxcrypted_quickdeploy_enable 1; billflxcrypted_bought_quickdeploy 1");
ClientCmd("exec touch/notice; touch_addbutton \"notice_bg2\" \"#SUCCESS\" \"\" 0.360000 0.555870 0.660000 0.684148 255 255 255 214 4; play media/PB_15ver_Equip_on.wav");
}

void CHudKillEffect::UserCmd_CommandActivebuy_qr(void)
{
ClientCmd("billflxcrypted_quickreload_enable 1; billflxcrypted_bought_quickreload 1");
ClientCmd("exec touch/notice; touch_addbutton \"notice_bg2\" \"#SUCCESS\" \"\" 0.360000 0.555870 0.660000 0.684148 255 255 255 214 4; play media/PB_15ver_Equip_on.wav");
}

void CHudKillEffect::UserCmd_CommandActivebuy_megahp(void)
{
ClientCmd("billflxcrypted_megahp_enable 1; billflxcrypted_bought_megahp 1");
ClientCmd("exec touch/notice; touch_addbutton \"notice_bg2\" \"#SUCCESS\" \"\" 0.360000 0.555870 0.660000 0.684148 255 255 255 214 4; play media/PB_15ver_Equip_on.wav");
}

void CHudKillEffect::UserCmd_CommandActivebuy_bpoint(void)
{
if (pb_point->value >= 0)
{
switch (Com_RandomLong(1, 20))
{
case 1: pb_point->value += 20000; break;
case 2: pb_point->value += 5000; break;
case 3: pb_point->value += 20000; break;
default: pb_point->value += 1000; break;
}
gEngfuncs.Cvar_SetValue( "billflxencrypted_pb_points", pb_point->value);
ClientCmd("exec touch/notice; touch_addbutton \"notice_bg2\" \"#POINTS GIVEN\" \"\" 0.360000 0.555870 0.660000 0.684148 255 255 255 214 4; play media/PB_15ver_Equip_on.wav");
}
}

// MsgFuncs for Shop
int CHudKillEffect::MsgFunc_buy_qc(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_qrespawn(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_megahp(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_bpoint(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_qr(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_Unequip_mask(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_Count_unit(const char *pszName, int iSize, void *pbuf ) { return 1; }

// Mask Functions
void CHudKillEffect::UserCmd_CommandActivebuy_mask_1(void) { ClientCmd("billflxcrypted_item_mask 1"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_2(void) { ClientCmd("billflxcrypted_item_mask 2"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_3(void) { ClientCmd("billflxcrypted_item_mask 3"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_4(void) { ClientCmd("billflxcrypted_item_mask 4"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_5(void) { ClientCmd("billflxcrypted_item_mask 5"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_6(void) { ClientCmd("billflxcrypted_item_mask 6"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_7(void) { ClientCmd("billflxcrypted_item_mask 7"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_8(void) { ClientCmd("billflxcrypted_item_mask 8"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_9(void) { ClientCmd("billflxcrypted_item_mask 9"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_10(void) { ClientCmd("billflxcrypted_item_mask 10"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_11(void) { ClientCmd("billflxcrypted_item_mask 11"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_12(void) { ClientCmd("billflxcrypted_item_mask 12"); }
void CHudKillEffect::UserCmd_CommandActivebuy_mask_13(void) { ClientCmd("billflxcrypted_item_mask 13"); }

int CHudKillEffect::MsgFunc_buy_mask_1(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_2(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_3(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_4(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_5(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_6(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_7(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_8(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_9(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_10(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_11(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_12(const char *pszName, int iSize, void *pbuf ) { return 1; }
int CHudKillEffect::MsgFunc_buy_mask_13(const char *pszName, int iSize, void *pbuf ) { return 1; }
