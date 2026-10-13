// Pause menu Catalog page (Catalogsanity): one pool at a time (left/right), its
// items in two columns sorted by id, a star and white on the ones in the catalog.
// Up/down scrolls by row. A locked pool shows only "Locked".
#ifndef PC_AP_CATALOG_MENU_H
#define PC_AP_CATALOG_MENU_H

#include "types.h"
#include "ap_slotdata.h" // AP_CATALOG_MAX
#include "m_item_name.h"
#include "pc_menu_list.h"
#include "pc_menu_util.h" // pc_menu_action

#ifdef __cplusplus
extern "C" {
#endif

struct game_s;

typedef struct {
  int pool;          // 0-based
  pc_menu_list list; // items of pool (none when locked)
  mActor_name_t items[AP_CATALOG_MAX];
  char names[AP_CATALOG_MAX][mIN_ITEM_NAME_LEN + 1]; // game font, trailing spaces cut
} pc_ap_catalog_menu;

// Opens on the first open pool that still has checks
void pc_ap_catalog_menu_enter(pc_ap_catalog_menu* m);

// Up/down scroll, left/right page, cancel = PC_MENU_BACK; else PC_MENU_STAY
int pc_ap_catalog_menu_input(pc_ap_catalog_menu* m, pc_menu_action act);

// Draws into NOW_FONT_DISP with its own dim backdrop. Font projection must be loaded.
void pc_ap_catalog_menu_draw(const pc_ap_catalog_menu* m, struct game_s* game);

#ifdef __cplusplus
}
#endif

#endif
