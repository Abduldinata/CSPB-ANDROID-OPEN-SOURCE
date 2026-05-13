#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "weapons.h"
#include "weapons_precache.h"

#ifndef CSPB_LOG_DIAG
#define CSPB_LOG_DIAG(msg) ALERT(at_console, "%s\n", msg)
#endif

// Keep this symbol for compatibility if other files reference it.
bool g_bCSPBStartupWeaponPrecachePhase = false;

// called by worldspawn
void W_Precache()
{
	CSPB_LOG_DIAG("=== W_Precache START ===");
	Q_memset(CBasePlayerItem::ItemInfoArray, 0, ARRAYSIZE(CBasePlayerItem::ItemInfoArray));
	Q_memset(CBasePlayerItem::AmmoInfoArray, 0, ARRAYSIZE(CBasePlayerItem::AmmoInfoArray));
	giAmmoIndex = 0;
	g_bCSPBStartupWeaponPrecachePhase = true;

	// common world objects
	UTIL_PrecacheOther("item_suit");
	UTIL_PrecacheOther("item_battery");
	UTIL_PrecacheOther("item_antidote");
	UTIL_PrecacheOther("item_security");
	UTIL_PrecacheOther("item_longjump");
	UTIL_PrecacheOther("item_kevlar");
	UTIL_PrecacheOther("item_assaultsuit");
	UTIL_PrecacheOther("item_thighpack");

	// awp magnum
	UTIL_PrecacheOtherWeapon("weapon_awp");
	UTIL_PrecacheOther("ammo_338magnum");

	
	UTIL_PrecacheOtherWeapon("weapon_ak47");
	UTIL_PrecacheOtherWeapon("weapon_scout");
	UTIL_PrecacheOther("ammo_762nato");

	// m249
	
	UTIL_PrecacheOther("ammo_556natobox");

	UTIL_PrecacheOtherWeapon("weapon_m4a1");

	UTIL_PrecacheOtherWeapon("weapon_aug");
	UTIL_PrecacheOtherWeapon("weapon_sg550");
	UTIL_PrecacheOther("ammo_556nato");

	// shotgun
	UTIL_PrecacheOtherWeapon("weapon_m3");
		UTIL_PrecacheOther("ammo_buckshot");

	UTIL_PrecacheOtherWeapon("weapon_usp");
	
	UTIL_PrecacheOther("ammo_45acp");

	UTIL_PrecacheOtherWeapon("weapon_p90");
	UTIL_PrecacheOther("ammo_57mm");

	// deagle
	UTIL_PrecacheOtherWeapon("weapon_deagle");
	UTIL_PrecacheOther("ammo_50ae");


	// knife
	UTIL_PrecacheOtherWeapon("weapon_flashbang");
	UTIL_PrecacheOtherWeapon("weapon_hegrenade");
	UTIL_PrecacheOtherWeapon("weapon_smokegrenade");
	UTIL_PrecacheOtherWeapon("weapon_c4");

#ifdef __ANDROID__
	CSPB_LOG_DIAG("[DIAG] Android recovery: skipping optional heavy startup weapon precache tail; assets will late-precache in-game");
#else
	UTIL_PrecacheOtherWeapon("weapon_colt_python");
	UTIL_PrecacheOtherWeapon("weapon_deagle_dual");
	UTIL_PrecacheOtherWeapon("weapon_dual_handgun");
	UTIL_PrecacheOtherWeapon("weapon_taurus_raging_bull");
	UTIL_PrecacheOtherWeapon("weapon_deagle");
	UTIL_PrecacheOtherWeapon("weapon_usp");
	UTIL_PrecacheOtherWeapon("weapon_glock18");
	UTIL_PrecacheOtherWeapon("weapon_bow");
	UTIL_PrecacheOtherWeapon("weapon_ak47");
	UTIL_PrecacheOtherWeapon("weapon_aksopmod");
	UTIL_PrecacheOtherWeapon("weapon_aug_hbar");
	UTIL_PrecacheOtherWeapon("weapon_aug");
	UTIL_PrecacheOtherWeapon("weapon_augblitz");
	UTIL_PrecacheOtherWeapon("weapon_p90");
	UTIL_PrecacheOtherWeapon("weapon_aug_a3_silencer");
	UTIL_PrecacheOtherWeapon("weapon_f2000");
	UTIL_PrecacheOtherWeapon("weapon_famas_g2");
	UTIL_PrecacheOtherWeapon("weapon_g36c");
	UTIL_PrecacheOtherWeapon("weapon_k1");
	UTIL_PrecacheOtherWeapon("weapon_k2");
	UTIL_PrecacheOtherWeapon("weapon_kriss_sv");
	UTIL_PrecacheOtherWeapon("weapon_kriss_sv_dual");
	UTIL_PrecacheOtherWeapon("weapon_kriss_sv_silence");
	UTIL_PrecacheOtherWeapon("weapon_kriss_sv_dual_silence");
	UTIL_PrecacheOtherWeapon("weapon_m4_cqb_lv1");
	UTIL_PrecacheOtherWeapon("weapon_m4_cqb_lv2");
	UTIL_PrecacheOtherWeapon("weapon_m4a1");
	UTIL_PrecacheOtherWeapon("weapon_m4a1_s");
	UTIL_PrecacheOtherWeapon("weapon_mp7");
	UTIL_PrecacheOtherWeapon("weapon_oa93");
	UTIL_PrecacheOtherWeapon("weapon_oa93_dual");
	UTIL_PrecacheOtherWeapon("weapon_p90_mc");
	UTIL_PrecacheOtherWeapon("weapon_pindad_ss2_v5");
	UTIL_PrecacheOtherWeapon("weapon_groza");
	UTIL_PrecacheOtherWeapon("weapon_sc2010");
	UTIL_PrecacheOtherWeapon("weapon_scar_carbine");
	UTIL_PrecacheOtherWeapon("weapon_kriss_sv_crb");
	UTIL_PrecacheOtherWeapon("weapon_m4a1_s");
	UTIL_PrecacheOtherWeapon("weapon_mp5k");
	UTIL_PrecacheOtherWeapon("weapon_m4_azure");
	UTIL_PrecacheOtherWeapon("weapon_mp9");
	UTIL_PrecacheOtherWeapon("weapon_sg550");
	UTIL_PrecacheOtherWeapon("weapon_awp");
	UTIL_PrecacheOtherWeapon("weapon_cheytac_m200");
	UTIL_PrecacheOtherWeapon("weapon_dragunov");
	UTIL_PrecacheOtherWeapon("weapon_kar98k");
	UTIL_PrecacheOtherWeapon("weapon_rangemaster_338");
	UTIL_PrecacheOtherWeapon("weapon_m82a1");
	UTIL_PrecacheOtherWeapon("weapon_tactilite_t2");
	UTIL_PrecacheOtherWeapon("weapon_scout");
	UTIL_PrecacheOtherWeapon("weapon_m4_spr_lv1");
	UTIL_PrecacheOtherWeapon("weapon_m4_spr_lv2");
	UTIL_PrecacheOtherWeapon("weapon_m1887");
	UTIL_PrecacheOtherWeapon("weapon_spas_15");
	UTIL_PrecacheOtherWeapon("weapon_zombie_s");
	UTIL_PrecacheOtherWeapon("weapon_m3");
#endif

	CSPB_LOG_DIAG("[DIAG] Precache block #46-49 START");
#ifndef CSPB_SKIP_HEAVY_PRECACHE_BLOCK
#define CSPB_SKIP_HEAVY_PRECACHE_BLOCK 1
#endif
#if CSPB_SKIP_HEAVY_PRECACHE_BLOCK
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_aksopmod_cg");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_aug_esport");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_t77");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_t77_dual");
#else
	UTIL_PrecacheOtherWeapon("weapon_aksopmod_cg");
	CSPB_LOG_DIAG("[DIAG] weapon_aksopmod_cg DONE");
	UTIL_PrecacheOtherWeapon("weapon_aug_esport");
	CSPB_LOG_DIAG("[DIAG] weapon_aug_esport DONE");
	UTIL_PrecacheOtherWeapon("weapon_t77");
	UTIL_PrecacheOtherWeapon("weapon_t77_dual");
	CSPB_LOG_DIAG("[DIAG] weapon_t77/t77_dual DONE");
#endif

#ifndef CSPB_ENABLE_WEAPON_APC
#define CSPB_ENABLE_WEAPON_APC 0
#endif
#if CSPB_ENABLE_WEAPON_APC
	UTIL_PrecacheOtherWeapon("weapon_apc");
	CSPB_LOG_DIAG("[DIAG] weapon_apc DONE");
#else
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_apc (define CSPB_ENABLE_WEAPON_APC=1 to enable)");
#endif

	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_fg42");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_msbs");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_as50");

#if CSPB_SKIP_HEAVY_PRECACHE_BLOCK
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_m1887_w");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_pgm");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_ump");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_sig");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_spectre");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_tar");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_xm8");
	CSPB_LOG_DIAG("[DIAG] skipped heavy block #53-59 DONE");
#else
	UTIL_PrecacheOtherWeapon("weapon_m1887_w");
	CSPB_LOG_DIAG("[DIAG] weapon_m1887_w DONE");
	UTIL_PrecacheOtherWeapon("weapon_pgm");
	CSPB_LOG_DIAG("[DIAG] weapon_pgm DONE");
	UTIL_PrecacheOtherWeapon("weapon_ump");
	CSPB_LOG_DIAG("[DIAG] weapon_ump DONE");
	UTIL_PrecacheOtherWeapon("weapon_sig");
	CSPB_LOG_DIAG("[DIAG] weapon_sig DONE");
	UTIL_PrecacheOtherWeapon("weapon_spectre");
	CSPB_LOG_DIAG("[DIAG] weapon_spectre DONE");
	UTIL_PrecacheOtherWeapon("weapon_tar");
	CSPB_LOG_DIAG("[DIAG] weapon_tar DONE");
	UTIL_PrecacheOtherWeapon("weapon_xm8");
	CSPB_LOG_DIAG("[DIAG] weapon_xm8 DONE");
#endif

	UTIL_PrecacheOtherWeapon("weapon_water");
#ifndef CSPB_SKIP_MELEE_PRECACHE_TAIL
#define CSPB_SKIP_MELEE_PRECACHE_TAIL 1
#endif
#if CSPB_SKIP_MELEE_PRECACHE_TAIL
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_knife");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_amok");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_saber");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_arabian_sword");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_fangblade");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_combat");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_knifebone");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_brass_knuckle");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_candy_cane");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_dual_knife");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_keris");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_mini_axe");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_knife (duplicate)");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_ice");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_karambit");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_butterfly");
	CSPB_LOG_DIAG("[DIAG] skipped melee tail DONE");
#else
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_knife");
	UTIL_PrecacheOtherWeapon("weapon_amok");
	UTIL_PrecacheOtherWeapon("weapon_saber");
	UTIL_PrecacheOtherWeapon("weapon_arabian_sword");
	UTIL_PrecacheOtherWeapon("weapon_fangblade");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_combat");
	UTIL_PrecacheOtherWeapon("weapon_knifebone");
	UTIL_PrecacheOtherWeapon("weapon_brass_knuckle");
	UTIL_PrecacheOtherWeapon("weapon_candy_cane");
	UTIL_PrecacheOtherWeapon("weapon_dual_knife");
	UTIL_PrecacheOtherWeapon("weapon_keris");
	UTIL_PrecacheOtherWeapon("weapon_mini_axe");
	CSPB_LOG_DIAG("[DIAG] SKIPPED: weapon_knife (duplicate)");
	UTIL_PrecacheOtherWeapon("weapon_ice");
	UTIL_PrecacheOtherWeapon("weapon_karambit");
	UTIL_PrecacheOtherWeapon("weapon_butterfly");
#endif
	UTIL_PrecacheOtherWeapon("weapon_hegrenade");
	UTIL_PrecacheOtherWeapon("weapon_gasbomb");
	UTIL_PrecacheOtherWeapon("weapon_smokegrenade");
#ifdef __ANDROID__
	CSPB_LOG_DIAG("[DIAG] Android recovery: skipped weapon_medkit startup precache");
#else
	UTIL_PrecacheOtherWeapon("weapon_medkit");
#endif

	if (g_pGameRules->IsDeathmatch())
	{
		// container for dropped deathmatch weapons
		UTIL_PrecacheOther("weaponbox");
	}

	g_sModelIndexFireball = PRECACHE_MODEL("sprites/zerogxplode.spr");	// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL("sprites/WXplo1.spr");		// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL("sprites/steam1.spr");		// smoke
	g_sModelIndexBubbles = PRECACHE_MODEL("sprites/bubble.spr");		// bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL("sprites/bloodspray.spr");	// initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL("sprites/blood.spr");		// splattered blood

	g_sModelIndexSmokePuff = PRECACHE_MODEL("sprites/smokepuff.spr");
	g_sModelIndexFireball2 = PRECACHE_MODEL("sprites/eexplo.spr");
	g_sModelIndexFireball3 = PRECACHE_MODEL("sprites/fexplo.spr");
	g_sModelIndexFireball4 = PRECACHE_MODEL("sprites/fexplo1.spr");
	g_sModelIndexRadio = PRECACHE_MODEL("sprites/radio.spr");

	g_sModelIndexCTGhost = PRECACHE_MODEL("sprites/b-tele1.spr");
	g_sModelIndexTGhost = PRECACHE_MODEL("sprites/c-tele1.spr");
	g_sModelIndexC4Glow = PRECACHE_MODEL("sprites/ledglow.spr");

	g_sModelIndexLaser = PRECACHE_MODEL((char*)g_pModelNameLaser);
	g_sModelIndexLaserDot = PRECACHE_MODEL("sprites/laserdot.spr");

	// used by explosions
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_MODEL("sprites/explode1.spr");

	PRECACHE_SOUND("weapons/debris1.wav");		// explosion aftermaths
	PRECACHE_SOUND("weapons/debris2.wav");		// explosion aftermaths
	PRECACHE_SOUND("weapons/debris3.wav");		// explosion aftermaths

	PRECACHE_SOUND("weapons/grenade_hit1.wav");	// grenade
	PRECACHE_SOUND("weapons/grenade_hit2.wav");	// grenade
	PRECACHE_SOUND("weapons/grenade_hit3.wav");	// grenade

	PRECACHE_SOUND("weapons/bullet_hit1.wav");	// hit by bullet
	PRECACHE_SOUND("weapons/bullet_hit2.wav");	// hit by bullet

	PRECACHE_SOUND("items/weapondrop1.wav");	// weapon falls to the ground
	PRECACHE_SOUND("weapons/generic_reload.wav");

	g_bCSPBStartupWeaponPrecachePhase = false;
	CSPB_LOG_DIAG("=== W_Precache END - All precache complete ===");
}
