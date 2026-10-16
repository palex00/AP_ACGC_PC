// Shellsanity: Archipelago items wash up on the town beach in place of shells. Each month has
// `per_month` checks; picking up a beach item sends the next unsent check of its month. No more
// of a month's items lie on the beach than it has checks left. Nothing is saved: a beach item
// whose check got lost (quit before it reached the server) is simply replaced by a new one.
// Reads AP state (checked locations), never writes to the DLL or the game: the pc_ap_logic.c
// facade places items and sends the ids these return.
#ifndef PC_AP_SHELLSANITY_H
#define PC_AP_SHELLSANITY_H

#include "types.h"
#include "m_actor_type.h"
#include "m_name_table.h"
#include "ap_slotdata.h"

#ifdef __cplusplus
extern "C" {
#endif

// WARN: keep in sync with apworld shellsanity.py! Month m (0-11), check k (1-based) = base + m * 0x100 + k
#define PC_AP_LOC_BEACH_BASE 0x50000
#define PC_AP_MONTHS         12

// Beach item of month m (0-11) = start + m: free ids after the ETC items (ETC_NUM 49, 0x2500-0x2530)
#define PC_AP_BEACH_ITEM_START (ITM_ETC_START + 0x31)
#define PC_AP_BEACH_ITEM_END   (PC_AP_BEACH_ITEM_START + PC_AP_MONTHS)

// mNT_get_itemTableNo result for beach items. The bg_item draw types are the item types plus
// bIT_TYPE_BASE, then FISH2: ours comes right after FISH2 (static assert in pc_ap_beach_draw.c_inc)
#define PC_AP_ITEM_TYPE_BEACH (mNT_ITEM_TYPE_NUM + 1)

typedef struct {
  // Config (slot_data)
  int per_month; // checks per month, 0 = off
  int chance;    // percent of washed-up shells that become beach items (Settings > AP)
} pc_ap_shellsanity;

// Config from slot_data
void pc_ap_shellsanity_init(pc_ap_shellsanity* s, const ap_slotdata* sd);

// Nonzero for a beach item id (any month)
int pc_ap_shellsanity_is_item(mActor_name_t item);

// Location id of month (0-11), check k (1-based)
int64_t pc_ap_shellsanity_location(int month, int k);

// Checks of month (0-11) not sent yet
int pc_ap_shellsanity_left(const pc_ap_shellsanity* s, int month);

// What washes up: the beach item of month (0-11) when the roll (0-99) hits and fewer than its
// checks left lie on the beach (on_beach), else shell
mActor_name_t pc_ap_shellsanity_wash_up(const pc_ap_shellsanity* s, mActor_name_t shell, int month, int on_beach,
                                        int roll);

// Picking up beach item: the location to send (next unsent check of its month), -1 if none left
int64_t pc_ap_shellsanity_pickup(const pc_ap_shellsanity* s, mActor_name_t item);

// Item name of beach item in ASCII ("AP Shell Jan"), padded with spaces to len chars;
// out holds len + 1 (NUL-terminated)
void pc_ap_shellsanity_name(mActor_name_t item, char* out, int len);

#ifdef __cplusplus
}
#endif

#endif
