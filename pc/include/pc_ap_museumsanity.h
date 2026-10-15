// Museumsanity: museum donations are AP items, the museum shows the received
// ones; donating (Blathers or mail) and finding things are checks.
// Reads AP state (received items, checked locations), never writes to the DLL;
// the only game state it changes is the museum bits passed in. The
// pc_ap_logic.c facade sends the ids these return.
// A category with no checks (or museumsanity off, or offline) is fully vanilla.
#ifndef PC_AP_MUSEUMSANITY_H
#define PC_AP_MUSEUMSANITY_H

#include "types.h"
#include "m_museum_display.h"
#include "ap_slotdata.h"
#include "ac_set_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// WARN: keep in sync with apworld museum.py! Museum slot = category
// (mMmd_CATEGORY_*) * 0x40 + museum index; the item and both locations use it.
#define PC_AP_MUSEUM_SLOT(cat, idx)  ((cat) * 0x40 + (idx))
#define PC_AP_MUSEUM_SLOT_NUM        (mMmd_CATEGORY_NUM * 0x40)
#define PC_AP_ITEM_MUSEUM_BASE       0x10100 // "Museum: X"
#define PC_AP_LOC_DONATE_BASE        0x30000 // "Donate X"
#define PC_AP_LOC_FIND_BASE          0x30100 // "Catch X" / "Dig Up X"
// WARN: keep in sync with apworld museum.py FOSSIL_SERIES! Progressive fossil
// spawns: "<series> Fossils" = base + series (Tricera, T-rex, Apato, Stego,
// Ptera, Plesio, Mammoth)
#define PC_AP_ITEM_FOSSIL_SERIES_BASE 0x10030
#define PC_AP_FOSSIL_SERIES_NUM       7
// WARN: keep in sync with apworld museum.py FISH_PASS_ITEMS! Fish Availability
// "Progressive": pass p item = base + p
#define PC_AP_ITEM_FISH_PASS_BASE     0x10040
#define PC_AP_FISH_PASS_SMALL_RIVER   0
#define PC_AP_FISH_PASS_MEDIUM_RIVER  1
#define PC_AP_FISH_PASS_LARGE_RIVER   2
#define PC_AP_FISH_PASS_POND          3 // ponds + the river lake's own fish
#define PC_AP_FISH_PASS_SEA           4
#define PC_AP_FISH_PASS_NUM           5

typedef struct {
  // Config (slot_data); all 0 when museumsanity is off or offline
  int checks[mMmd_CATEGORY_NUM]; // AP_MUSEUM_* bits per category, 0 = vanilla
  int goal_count;                // Museum goal: donation items to receive
  // Spawns: set even when museumsanity is off and offline
  int critter_odds;                      // AP_CRITTER_ODDS_*
  int fish_availability;                   // AP_FISH_AVAIL_* (progressive without fish checks = all)
  int fossil_availability;                 // AP_FOSSIL_AVAIL_* (progressive without fossil checks = all)
  int fossil_odds;                         // AP_FOSSIL_ODDS_*
  int fossil_seasons[mMmd_FOSSIL_NUM];     // season per fossil (0 spring .. 3 winter)
} pc_ap_museumsanity;

// Dynamic spawns: wanted things are this many times as likely as vanilla
#define PC_AP_SPAWN_BOOST 4

// Bugs and fish per category (museum index = journal index = spawn table type)
#define PC_AP_CRITTER_NUM 40

void pc_ap_museumsanity_init(pc_ap_museumsanity* m, const ap_slotdata* sd);

// Museum thing -> category + index (the museum display's numbering). 0 if the
// item isn't one (cat/idx untouched)
int pc_ap_museumsanity_slot_of(mActor_name_t item, int* cat, int* idx);
// Nonzero if the category is run by AP (has checks); 0 = vanilla
int pc_ap_museumsanity_active(const pc_ap_museumsanity* m, int cat);

// Donation items received for active categories, each species once
int pc_ap_museumsanity_received(const pc_ap_museumsanity* m);
// Same, also split per category (counts[cat], 0 for inactive ones)
int pc_ap_museumsanity_received_per_category(const pc_ap_museumsanity* m, int counts[mMmd_CATEGORY_NUM]);
// Find and donate checks of a category sent so far; *total = its number of checks (0 if inactive)
int pc_ap_museumsanity_checked(const pc_ap_museumsanity* m, int cat, int* total);
// Things in a category (museum display size)
int pc_ap_museumsanity_size(int cat);
// Make the bits of every active category match the received items: received =
// donated by player 1 (a real player code, so plaques name the donator; see
// pc_ap_museumsanity_sender), everything else cleared. Inactive categories are
// untouched. Returns pc_ap_museumsanity_received.
int pc_ap_museumsanity_sync(const pc_ap_museumsanity* m, mMmd_info_c* info);

// Donate location id if the museum should take it now (category has donate
// checks, not checked yet), else -1 (refuse)
int64_t pc_ap_museumsanity_donate_check(const pc_ap_museumsanity* m, int cat, int idx);
// Catch / dig up location id if the category has find checks and it isn't
// checked yet, else -1
int64_t pc_ap_museumsanity_find_check(const pc_ap_museumsanity* m, int cat, int idx);
// Slot (player) that sent the donation item, -1 if not received or inactive
int pc_ap_museumsanity_sender(const pc_ap_museumsanity* m, int cat, int idx);

// Nonzero if dug-up fossils come out appraised (fossils have find checks)
int pc_ap_museumsanity_instant_fossils(const pc_ap_museumsanity* m);
// Fossils dynamic spawns favour: wanted[idx] = 1 while its find or donate check
// is unsent (fossils active), else while it isn't on display in info
void pc_ap_museumsanity_fossil_wanted(const pc_ap_museumsanity* m, const mMmd_info_c* info,
                                      u8 wanted[mMmd_FOSSIL_NUM]);
// Progressive fossils: series (0-6) of a fossil (museum index), -1 for
// the always-open ones (Amber, Dinosaur Track, Ammonite, Dinosaur Egg, Trilobite)
int pc_ap_museumsanity_fossil_series(int idx);
// Nonzero if a fossil can come out: always, unless progressive and its
// series item isn't received
int pc_ap_museumsanity_fossil_open(const pc_ap_museumsanity* m, int idx);
// Progressive fossils: find and donate checks of the open fossils (sent or not),
// -1 when not progressive
int pc_ap_museumsanity_fossil_available(const pc_ap_museumsanity* m);
// Fossil (museum index) a dug-up fossil turns out to be: fossil_odds over the
// fossils fossil_availability allows. month 0-11, r random in [0, 1)
int pc_ap_museumsanity_pick_fossil(const pc_ap_museumsanity* m, const u8 wanted[mMmd_FOSSIL_NUM], int month,
                                   f32 r);

// Bugs or fish (cat) dynamic spawns favour: wanted[idx] = 1 while its catch or
// donate check is unsent (category active), else while it's missing from the
// journal (journal[idx] = 0) or from the museum display in info
void pc_ap_museumsanity_critter_wanted(const pc_ap_museumsanity* m, int cat, const mMmd_info_c* info,
                                       const u8 journal[PC_AP_CRITTER_NUM], u8 wanted[PC_AP_CRITTER_NUM]);
// Bugs/Fish pause pages: how many of its "has it" sources a bug or fish (cat, idx)
// has: 0 none, 1 has one, 2 has both (gold). Active category: its find / donate
// checks sent (one check mode -> 0 or 1). Inactive: journal (caught) and museum
// display (donated, displayed nonzero).
int pc_ap_museumsanity_critter_status(const pc_ap_museumsanity* m, int cat, int idx, int journal, int displayed);
// critter_odds on the game's spawn list right before the weighted draw, in
// place. Only the species' weights change (whale, trash, spirits, island "nothing"
// rows untouched) and their total stays the same, so the "nothing spawns" chance
// stays vanilla. Normalized: equal per species. Dynamic: normalized, then wanted ones
// x PC_AP_SPAWN_BOOST.
void pc_ap_museumsanity_fish_spawns(const pc_ap_museumsanity* m, const u8 wanted[PC_AP_CRITTER_NUM],
                                    aSOG_gyoei_spawn_info_weight_f_c* rows, int n);
// Fish Availability: nonzero if fish pass p is open (always unless progressive)
int pc_ap_museumsanity_fish_pass_open(const pc_ap_museumsanity* m, int pass);
// Fish pass of one spawn row (aGYO_TYPE_*, aSOG_SPAWN_AREA_*): river, waterfall and
// river mouth by the fish's size, lake and ponds = pond, sea = sea. -1 for rows that
// aren't a museum fish or are left alone (whale, offing, trash)
int pc_ap_museumsanity_fish_row_pass(int type, int area);
// Fish Availability on the spawn list (after the odds): a row whose pass is closed keeps
// its weight but gets an invalid spawn area, so picking it spawns nothing (the others
// keep vanilla odds). Not for the island (its rows are tagged sea too).
void pc_ap_museumsanity_fish_lock(const pc_ap_museumsanity* m, aSOG_gyoei_spawn_info_weight_f_c* rows, int n);
void pc_ap_museumsanity_insect_spawns(const pc_ap_museumsanity* m, const u8 wanted[PC_AP_CRITTER_NUM],
                                      aSOI_insect_spawn_info_f_c* rows, int n);

#ifdef __cplusplus
}
#endif

#endif
