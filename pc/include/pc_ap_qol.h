// Quality-of-life options: Settings > AP (pc_ap_settings.h) for the ones the player can
// change, slot_data (apworld options.py) for the rest.
#ifndef PC_AP_QOL_H
#define PC_AP_QOL_H

#ifdef __cplusplus
extern "C" {
#endif

// Nonzero when the train intro and Nook's job should be skipped (then
// aNPS_setup_game_start calls pc_ap_newgame instead of going to the train).
int pc_ap_qol_skip_intro(void);

// Nonzero when weeds should never grow. Hooked in mAGrw_RenewalFgItem_ovl
// by setting the Wisp's clear_grass flag, which clears all weeds and skips
// growing new ones.
int pc_ap_qol_no_weeds(void);

// Removes all cockroaches (every house and the island cottage) when the
// option is on. Hooked at the end of mCkRh_DecideNowGokiFamilyCount, which
// runs when a game is started or continued.
void pc_ap_qol_cockroaches(void);

// Nonzero when Nook's shop and Able Sisters ignore their opening hours.
// Hooked in mSP_ShopOpen (out-of-hours PRE/END become OPEN; first job,
// renewal and sale-day event statuses stay vanilla) and aNW_check_opend.
int pc_ap_qol_shops_always_open(void);

// Nonzero when villagers should (almost) always offer a favor. Hooked in
// aQMgr_actor_talk_select_talk (skips the "asked recently" cooldown) and
// aQMgr_actor_decide_quest (skips the 75% roll, re-rolls kinds that can't
// happen now). Caps and full pockets stay vanilla.
int pc_ap_qol_more_favors(void);

// Nonzero when villagers never move out. Hooked in mNpc_ForceRemove (no removal)
// and mNpc_SetRemoveAnimalNo (no villager picked to talk about moving; clears an
// already picked one, so talks fall through to the normal ones).
int pc_ap_qol_villagers_dont_leave(void);

// Nonzero when villagers never sleep. Hooked in mNPS_schedule_manager_sub: a SLEEP
// schedule entry becomes IN_HOUSE (awake at home, door open, talks and favors work).
int pc_ap_qol_villagers_never_sleep(void);

// Nonzero when turnips never spoil. Hooked in mAGrw_CheckSpoilKabuTime (weekly
// spoil) and mAGrw_ZuruSpoilKabu (going back in time).
int pc_ap_qol_turnips_never_spoil(void);

// Nonzero when the stalk market never rolls the falling pattern (type C).
// Hooked in Kabu_decide_trade_market.
int pc_ap_qol_no_falling_stalks(void);

// Nonzero when fishing never hooks trash. Hooked in the bite code of aGTT (fish)
// and aGKK (coelacanth), which swap the fish for a can, boot or tire 1 in 20.
int pc_ap_qol_no_fishing_trash(void);

// Nonzero when Nook's Happy Room Academy talk is skipped. Hooked in
// mEv_RenewalDataEveryDay: the "wait a day" flag is cleared without setting the talk flag.
int pc_ap_qol_skip_hra_talk(void);

// Nonzero when Nook's buy talk is short (Nook's shop / department store, not the twins).
// Hooked in ac_npc_shop_common.c (the raffle ticket talk and the clothes/carpet/wall
// follow-ups become the plain price offer) and pc_ap_msg_patch (filler lines trimmed).
int pc_ap_qol_short_shop_talk(void);

// Easier Critter Catch. Frames added to a fish's hook window (vanilla 20-90 at 60 Hz),
// 0 when off; hooked in aGTT_bite_init and aGKK_bite_init.
int pc_ap_qol_bite_window_bonus(void);
// Multiplier on how fast a bug's patience meter fills from nearby movement (flees at 90),
// 1 when off; hooked in aINS_calc_patience. Instant scares (net, tree shake) bypass it.
float pc_ap_qol_bug_stress_scale(void);
// Nonzero when running near a fish (110 units) scares it, 0 when Easier Critter Catch is on;
// hooked in aGTT_player_near and aGKK_player_near. Tool hits and the bobber still scare it.
int pc_ap_qol_fish_run_scare(void);

// Nonzero when hidden bugs give hints: a dirt clod over a buried mole cricket (aIKR_hide) or in
// front of a pill bug's rock (aIDG_hide), a small wiggle of the tree a bagworm or spider hides in
// (aIMN_hide).
int pc_ap_qol_hidden_bug_hints(void);

#ifdef __cplusplus
}
#endif

#endif
