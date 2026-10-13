// Paged list for pause menu pages: a header line, a status line, then items in
// rows x columns (row-major), scrolled one row at a time, "..." where more is
// above or below. The page draws the visible items itself:
//   for(int i = pc_menu_list_first(l); i < pc_menu_list_end(l); i++)
//     draw item i in column i % l->columns at pc_menu_list_y(l, i)
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

// New contents, back to the top
void pc_menu_list_set(pc_menu_list* l, int count, int columns, int rows);
void pc_menu_list_up(pc_menu_list* l);
void pc_menu_list_down(pc_menu_list* l);
int pc_menu_list_max_scroll(const pc_menu_list* l);

// Visible items: first .. end - 1, item i drawn at y
int pc_menu_list_first(const pc_menu_list* l);
int pc_menu_list_end(const pc_menu_list* l);
f32 pc_menu_list_y(const pc_menu_list* l, int i);

// Dim backdrop, header (white), status (gray if status_dim) and the "..." markers.
// Texts are in the game charset (pc_menu_fixtext). Draws into NOW_FONT_DISP; font
// projection must be loaded.
void pc_menu_list_draw(struct game_s* game, const pc_menu_list* l, const char* header, const char* status,
                       int status_dim);

#ifdef __cplusplus
}
#endif

#endif
