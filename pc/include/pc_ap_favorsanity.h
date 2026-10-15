// Favorsanity: a check for each of the first `count` villager favors, in pools of
// `per_pool`: the first pool is open, each Progressive Favor Pool opens the next.
// Favors done past the open pools still count (banked): their checks go out once
// their pool opens.
// Reads AP state (checked locations), never writes to the DLL or the game:
// the pc_ap_logic.c facade sends the ids these return.
#ifndef PC_AP_FAVORSANITY_H
#define PC_AP_FAVORSANITY_H

#include "types.h"
#include "ap_slotdata.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ini_t;

// WARN: keep in sync with apworld locations.py! Favor n (1-based) = base + n
#define PC_AP_LOC_FAVOR_BASE  0x20000
#define PC_AP_FAVORS_MAX      100
#define PC_AP_ITEM_PROGRESSIVE_FAVOR_POOL 0x10006

typedef struct {
  // Config (slot_data)
  int count;    // number of Favor checks
  int per_pool; // Favor checks per pool, 0 = all open
  // Persisted
  int done;  // favors completed, as of the last save
} pc_ap_favorsanity;

// Config from slot_data, persisted fields zeroed
void pc_ap_favorsanity_init(pc_ap_favorsanity* f, const ap_slotdata* sd);
void pc_ap_favorsanity_load(pc_ap_favorsanity* f, struct ini_t* ini);
void pc_ap_favorsanity_save(const pc_ap_favorsanity* f, struct ini_t* ini);

// Favors done = max(done, highest "Favor n" the server has or we sent this
// session), so an unsaved quit never makes you redo favors
int pc_ap_favorsanity_done(const pc_ap_favorsanity* f);
// Favor checks in the open pools (at most count)
int pc_ap_favorsanity_open(const pc_ap_favorsanity* f);
// Count one more completed favor. Returns its location id, or -1 past count or
// while its pool is closed (banked: next_check sends it once the pool opens)
int64_t pc_ap_favorsanity_complete(pc_ap_favorsanity* f);
// Next of the Favor 1..done location ids in open pools, -1 when done. Start *it
// at 0. For resending favors saved while offline and banked ones whose pool just
// opened; uses the local done only (the server already has anything above it).
int64_t pc_ap_favorsanity_next_check(const pc_ap_favorsanity* f, int* it);

#ifdef __cplusplus
}
#endif

#endif
