// Pause menu Catalog page (Catalogsanity): one pool at a time (left/right), its
// items in two columns sorted by id, a star and white on the ones in the catalog.
// Up/down scrolls by row. A locked pool shows only "Locked".
#ifndef PC_AP_CATALOG_MENU_H
#define PC_AP_CATALOG_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

struct game_s;

// Opens on the first open pool that still has checks
void pc_ap_catalog_menu_enter(void);

void pc_ap_catalog_menu_up(void);
void pc_ap_catalog_menu_down(void);
void pc_ap_catalog_menu_left(void);
void pc_ap_catalog_menu_right(void);

// Draws into NOW_FONT_DISP with its own dim backdrop. Font projection must be loaded.
void pc_ap_catalog_menu_draw(struct game_s* game);

#ifdef __cplusplus
}
#endif

#endif
