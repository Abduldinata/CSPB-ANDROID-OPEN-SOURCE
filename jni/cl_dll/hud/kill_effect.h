// pb sfuff

#pragma once

#include "r_texture.h"

class CHudKillEffect : public CHudBase {
public:
  int Init(void);
  int VidInit(void);
  int Draw(float flTime);
  virtual void Reset(void);
  void UpdateStars();
  void DrawFragRow();
  void AddFragToHistory(int fragIndex);

  CHudMsgFunc(killframe);
  CHudUserCmd(CommandActivekillframe);

  CHudMsgFunc(killframeAnim);
  CHudUserCmd(CommandActivekillframeAnim);

  CHudMsgFunc(PiercingShot);
  CHudUserCmd(CommandActivePiercingShot);

  CHudMsgFunc(MissionComplete);
  CHudUserCmd(CommandActiveMissionComplete);

  CHudMsgFunc(MassKill);
  CHudUserCmd(CommandActiveMassKill);

  CHudMsgFunc(Pointkill);
  CHudUserCmd(CommandActivePointkill);

  CHudMsgFunc(Doublekill);
  CHudUserCmd(CommandActiveDoublekill);

  CHudMsgFunc(Triplekill);
  CHudUserCmd(CommandActiveTriplekill);

  CHudMsgFunc(Chainkiller);
  CHudUserCmd(CommandActiveChainkiller);

  CHudMsgFunc(HeadshotPoint);
  CHudUserCmd(CommandActiveHeadshotPoint);

  CHudMsgFunc(Headshot);
  CHudUserCmd(CommandActiveHeadshot);

  CHudMsgFunc(ChainHeadshot);
  CHudUserCmd(CommandActiveChainHeadshot);

  CHudMsgFunc(Helmet);
  CHudUserCmd(CommandActiveHelmet);

  CHudMsgFunc(Stopper);
  CHudUserCmd(CommandActiveStopper);

  CHudMsgFunc(Slugger);
  CHudUserCmd(CommandActiveSlugger);

  CHudMsgFunc(PointNumber);
  CHudUserCmd(CommandActivePointNumber);

  CHudMsgFunc(HitMarker);
  CHudUserCmd(CommandActiveHitMarker);

  CHudMsgFunc(HotKiller);
  CHudUserCmd(CommandActiveHotKiller);

  CHudMsgFunc(Nightmare);
  CHudUserCmd(CommandActiveNightmare);

  CHudUserCmd(CommandActiveassist);

  // specific
  CHudMsgFunc(SpecialGunner);
  CHudUserCmd(CommandActiveSpecialGunner);

  CHudMsgFunc(BombShot);
  CHudUserCmd(CommandActiveBombShot);

  CHudMsgFunc(oneShot);
  CHudUserCmd(CommandActiveoneShot);

  CHudMsgFunc(OneshotEnable);
  CHudUserCmd(CommandActiveOneshotEnable);

  CHudMsgFunc(OneshotDisable);
  CHudUserCmd(CommandActiveOneshotDisable);

  CHudMsgFunc(FragAnimKill);
  CHudUserCmd(CommandActiveFragAnimKill);

  CHudMsgFunc(FragAnimHs);
  CHudUserCmd(CommandActiveFragAnimHs);

  CHudMsgFunc(FragAnimStopper);
  CHudUserCmd(CommandActiveFragAnimStopper);

  CHudMsgFunc(FragAnimStopperHs);
  CHudUserCmd(CommandActiveFragAnimStopperHs);

  CHudMsgFunc(FragAnimBlue);
  CHudUserCmd(CommandActiveFragAnimBlue);

  CHudMsgFunc(FragAnimGold);
  CHudUserCmd(CommandActiveFragAnimGold);
















  // item












  CHudMsgFunc(Add_point);
  CHudUserCmd(CommandActiveAdd_point);

public:
// Restored Shop Items
CHudMsgFunc(buy_qc);
CHudUserCmd(CommandActivebuy_qc);
CHudMsgFunc(buy_megahp);
CHudUserCmd(CommandActivebuy_megahp);
CHudMsgFunc(buy_bpoint);
CHudUserCmd(CommandActivebuy_bpoint);
CHudMsgFunc(buy_qr);
CHudUserCmd(CommandActivebuy_qr);
CHudMsgFunc(buy_mask_1);
CHudUserCmd(CommandActivebuy_mask_1);
CHudMsgFunc(buy_mask_2);
CHudUserCmd(CommandActivebuy_mask_2);
CHudMsgFunc(buy_mask_3);
CHudUserCmd(CommandActivebuy_mask_3);
CHudMsgFunc(buy_mask_4);
CHudUserCmd(CommandActivebuy_mask_4);
CHudMsgFunc(buy_mask_5);
CHudUserCmd(CommandActivebuy_mask_5);
CHudMsgFunc(buy_mask_6);
CHudUserCmd(CommandActivebuy_mask_6);
CHudMsgFunc(buy_mask_7);
CHudUserCmd(CommandActivebuy_mask_7);
CHudMsgFunc(buy_mask_8);
CHudUserCmd(CommandActivebuy_mask_8);
CHudMsgFunc(buy_mask_9);
CHudUserCmd(CommandActivebuy_mask_9);
CHudMsgFunc(buy_mask_10);
CHudUserCmd(CommandActivebuy_mask_10);
CHudMsgFunc(buy_mask_11);
CHudUserCmd(CommandActivebuy_mask_11);
CHudMsgFunc(buy_mask_12);
CHudUserCmd(CommandActivebuy_mask_12);
CHudMsgFunc(buy_mask_13);
CHudUserCmd(CommandActivebuy_mask_13);
CHudMsgFunc(buy_qrespawn);
CHudUserCmd(CommandActivebuy_qrespawn);
CHudMsgFunc(Unequip_mask);
CHudUserCmd(CommandActiveUnequip_mask);
CHudMsgFunc(Count_unit);
CHudUserCmd(CommandActiveCount_unit);

  int m_HUD_cross;

  long killframe_time;
  long killframeAnim_time;

  long Pointkill_time;
  long Doublekill_time;
  long Triplekill_time;
  long Chainkiller_time;
  long HeadshotPoint_time;
  long Headshot_time;
  long ChainHeadshot_time;
  long Helmet_time;
  long Stopper_time;
  long Slugger_time;
  long PointNumber_time;
  long HitMarker_time;
  long PiercingShot_time;
  long MassKill_time;

  long HotKiller_time;
  long Nightmare_time;
  long assist_time;

  // special
  long SpecialGunner_time;
  long BombShot_time;
  long oneShot_time;

  long FragAnimKill_time;
  long FragAnimHs_time;
  long FragAnimStopper_time;
  long FragAnimBlue_time;
  long FragAnimGold_time;








  long MissionComplete_time;



  long hit_time;
  long MeatChopper_time;

  // Frag Row History Tracking
  int m_fragHistory[10];
  float m_fragRowAnim[10];
  int m_fragCount;

  // V20 HUD Animation State
  int current_blood_frame;
  int last_frag_id;
  bool is_blood_anim_active;
  float m_center_anim_start_time;
  int m_pending_frag_id;
  bool m_pending_frag_added;

private:
  UniqueTexture m_killframe;

  UniqueTexture m_MissionComplete;

  UniqueTexture m_kill1[20];
  UniqueTexture m_hspoint[20];
  UniqueTexture m_kill2[20];
  UniqueTexture m_kill3[20];
  UniqueTexture m_kill4[20];
  UniqueTexture m_stopper[20];
  UniqueTexture m_helmet[20];
  UniqueTexture m_headshot[20];
  UniqueTexture m_chheadshot[20];
  UniqueTexture m_chslugger[20];
  UniqueTexture m_piercing[20];
  UniqueTexture m_mass[20];
  UniqueTexture m_point[4];
  UniqueTexture m_hitmarker;
  UniqueTexture m_hotkiller[20];
  UniqueTexture m_nightmare[20];
  UniqueTexture m_assist[1];
  UniqueTexture m_fraganim[20];
  // special
  UniqueTexture m_special[20];

  // underneath stars


  UniqueTexture m_hit;
};
