// Paged list for pause menu pages: a header line, a status line, then items in
// rows x columns (row-major), scrolled one row at a time, "..." where more is
// above or below. The page draws each visible item through a callback.
#ifndef PC_MENU_LIST_H
#define PC_MENU_LIST_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct game_s;

typedef struct {
  int count;   // items
  int columns; // items per row
  int rows;    // visible rows
  int scroll;  // first visible row
} pc_menu_list;

// Draws item (index into the page's items) in column at y
typedef void (*pc_menu_list_item_fn)(struct game_s* game, int item, int column, f32 y);

// New contents, back to the top
void pc_menu_list_set(pc_menu_list* l, int count, int columns, int rows);
void pc_menu_list_up(pc_menu_list* l);
void pc_menu_list_down(pc_menu_list* l);
int pc_menu_list_max_scroll(const pc_menu_list* l);

// Dim backdrop, header (white), status (gray if status_dim) and the visible items.
// Texts are in the game charset (pc_menu_fixtext). Draws into NOW_FONT_DISP; font
// projection must be loaded.
void pc_menu_list_draw(struct game_s* game, const pc_menu_list* l, const char* header, const char* status,
                       int status_dim, pc_menu_list_item_fn draw_item);

#ifdef __cplusplus
}
#endif

#endif
