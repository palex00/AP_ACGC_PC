// Nook Store Upgrades: what upgrades Nook's store (Progressive Nook Store items,
// total sales or catalog entries). Picks the store's target level; the game's own
// upgrade flow (notice, 2-day renovation, rebuild) does the rest. Never needs a
// visitor from another town. Reads AP state (received items), never writes to
// the DLL or the game. Offline (and old slot_data): vanilla.
#ifndef PC_AP_STORE_H
#define PC_AP_STORE_H

#include "types.h"
#include "m_private.h"
#include "ap_slotdata.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PC_AP_ITEM_PROGRESSIVE_NOOK_STORE 0x10004

typedef struct {
  // Config (slot_data)
  int mode;                          // AP_STORE_*
  u32 thresholds[AP_STORE_LEVELS];   // per upgrade: Bells (SALES) or catalog entries (CATALOG)
} pc_ap_store;

void pc_ap_store_init(pc_ap_store* s, const ap_slotdata* sd);

// Target store level (0-3) for mSP_GetRealShopLevel: at most cur_level + 1 (one
// renovation at a time), never below cur_level. -1 = vanilla (run the game's code).
int pc_ap_store_level(const pc_ap_store* s, int cur_level, u32 sales_sum, int catalog_count);
// Sales clamp for mSP_PlusSales (sales never run ahead of the next upgrade):
// 1 and *cap = the most sales_sum may be, 0 = vanilla clamp.
int pc_ap_store_sales_cap(const pc_ap_store* s, int cur_level, u32* cap);
// Catalog entries of one player: the collected bits, without the bug and fish
// journal (stored in the furniture bits but not shown in the catalog)
int pc_ap_store_catalog_count(const Private_c* priv);

#ifdef __cplusplus
}
#endif

#endif
