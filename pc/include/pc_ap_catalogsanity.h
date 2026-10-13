// Catalogsanity: catalog items are split into pools; an item of an open pool in
// the current player's catalog is a check. Pool 1 is open from the start, each
// Progressive Catalog Pool opens the next one. Checks are derived from the
// catalog bits (no hook on registering), so a pool opening sends the items
// already in the catalog at once.
// Reads AP state (received items) and the catalog, never writes to the DLL or
// the game: the pc_ap_logic.c facade sends the ids these return.
// Offline (and old slot_data): no pools, nothing happens.
#ifndef PC_AP_CATALOGSANITY_H
#define PC_AP_CATALOGSANITY_H

#include "types.h"
#include "m_actor_type.h"
#include "ap_slotdata.h"

#ifdef __cplusplus
extern "C" {
#endif

// WARN: keep in sync with apworld items.py / catalog.py!
#define PC_AP_ITEM_PROGRESSIVE_CATALOG_POOL 0x10005
#define PC_AP_LOC_CATALOG_BASE              0x40000 // "Catalog: X" = base + game item id

typedef struct {
  // Config (slot_data); count 0 when catalogsanity is off or offline
  int count;
  mActor_name_t items[AP_CATALOG_MAX]; // game item ids, pools flattened
  u8 pool[AP_CATALOG_MAX];             // pool (0-based) of each item
} pc_ap_catalogsanity;

void pc_ap_catalogsanity_init(pc_ap_catalogsanity* c, const ap_slotdata* sd);

// Pools open: 1 + Progressive Catalog Pools received (0 without catalogsanity)
int pc_ap_catalogsanity_open_pools(const pc_ap_catalogsanity* c);
// Next "Catalog: X" id of an item in the first `open` pools that is in the
// current player's catalog (mSP_CollectCheck), -1 when done. Start *it at 0.
// Includes ones already sent: the DLL drops repeats.
int64_t pc_ap_catalogsanity_next_check(const pc_ap_catalogsanity* c, int open, int* it);

#ifdef __cplusplus
}
#endif

#endif
