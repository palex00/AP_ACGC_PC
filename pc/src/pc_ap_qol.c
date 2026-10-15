#include "pc_ap_qol.h"
#include "ap_slotdata.h"
#include "pc_ap_logic.h"
#include "pc_ap_settings.h"

#include "m_common_data.h"
#include "m_home.h"

// slot_data skip_intro values (apworld options.py SkipIntro)
#define PC_AP_SKIP_INTRO_TRAIN_AND_JOB 2

int pc_ap_qol_skip_intro(void) {
  ap_slotdata* sd = ap_getslotdata();
  return sd->valid && sd->skip_intro == PC_AP_SKIP_INTRO_TRAIN_AND_JOB;
}

int pc_ap_qol_no_weeds(void) {
  return pc_ap_setting(PC_AP_SET_NO_WEEDS);
}

int pc_ap_qol_more_favors(void) {
  return pc_ap_setting(PC_AP_SET_MORE_FAVORS);
}

int pc_ap_qol_villagers_dont_leave(void) {
  return pc_ap_setting(PC_AP_SET_VILLAGERS_DONT_LEAVE);
}

int pc_ap_qol_villagers_never_sleep(void) {
  return pc_ap_setting(PC_AP_SET_VILLAGERS_NEVER_SLEEP);
}

int pc_ap_qol_turnips_never_spoil(void) {
  ap_slotdata* sd = ap_getslotdata();
  return sd->valid && sd->turnips_never_spoil;
}

int pc_ap_qol_no_falling_stalks(void) {
  return pc_ap_setting(PC_AP_SET_NO_FALLING_STALKS);
}

int pc_ap_qol_no_fishing_trash(void) {
  return pc_ap_setting(PC_AP_SET_NO_FISHING_TRASH);
}

int pc_ap_qol_skip_hra_talk(void) {
  return pc_ap_setting(PC_AP_SET_SKIP_HRA_TALK);
}

// Easier Critter Catch: +1 s to hook a fish, bugs notice you half as fast
#define PC_AP_QOL_BITE_WINDOW_BONUS 60
#define PC_AP_QOL_BUG_STRESS_SCALE 0.5f

static int pc_ap_qol_easier_critter_catch(void) {
  return pc_ap_setting(PC_AP_SET_EASIER_CRITTER_CATCH);
}

int pc_ap_qol_bite_window_bonus(void) {
  return pc_ap_qol_easier_critter_catch() ? PC_AP_QOL_BITE_WINDOW_BONUS : 0;
}

float pc_ap_qol_bug_stress_scale(void) {
  return pc_ap_qol_easier_critter_catch() ? PC_AP_QOL_BUG_STRESS_SCALE : 1.0f;
}

int pc_ap_qol_shops_always_open(void) {
  ap_slotdata* sd = ap_getslotdata();
  return sd->valid && sd->shops_always_open;
}

void pc_ap_qol_cockroaches(void) {
  int i;

  if (!pc_ap_setting(PC_AP_SET_NO_COCKROACHES)) {
    return;
  }

  for (i = 0; i < PLAYER_NUM; i++) {
    Save_Set(homes[i].goki.num, 0);
  }
  Save_Set(island.cottage.goki.num, 0);
}
