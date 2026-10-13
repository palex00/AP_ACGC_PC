#include "pc_menu_list.h"
#include "pc_menu_util.h"

#include "game.h"

#define PC_MENU_LIST_Y0     66.0f // first row
#define PC_MENU_LIST_LINE_H 14.0f

void pc_menu_list_set(pc_menu_list* l, int count, int columns, int rows) {
  l->count = count;
  l->columns = columns;
  l->rows = rows;
  l->scroll = 0;
}

int pc_menu_list_max_scroll(const pc_menu_list* l) {
  int rows = (l->count + l->columns - 1) / l->columns;
  return rows > l->rows ? rows - l->rows : 0;
}

void pc_menu_list_up(pc_menu_list* l) {
  if(l->scroll > 0) {
    l->scroll--;
  }
}

void pc_menu_list_down(pc_menu_list* l) {
  if(l->scroll < pc_menu_list_max_scroll(l)) {
    l->scroll++;
  }
}

void pc_menu_list_draw(struct game_s* game, const pc_menu_list* l, const char* header, const char* status,
                       int status_dim, pc_menu_list_item_fn draw_item) {
  pc_menu_dim_rect(game->graph, 180);
  pc_menu_draw_centered(game, header, 22.0f, 255, 255, 255, 255, 1.0f);
  if(status_dim) {
    pc_menu_draw_centered(game, status, 38.0f, 150, 150, 150, 220, 1.0f);
  } else {
    pc_menu_draw_centered(game, status, 38.0f, 230, 230, 230, 255, 1.0f);
  }

  int first = l->scroll * l->columns;
  for(int i = first; i < l->count && i < first + l->rows * l->columns; i++) {
    draw_item(game, i, i % l->columns, PC_MENU_LIST_Y0 + ((i - first) / l->columns) * PC_MENU_LIST_LINE_H);
  }

  // More above / below
  if(l->scroll > 0) {
    pc_menu_draw_centered(game, "...", PC_MENU_LIST_Y0 - 12.0f, 150, 150, 150, 200, 1.0f);
  }
  if(l->scroll < pc_menu_list_max_scroll(l)) {
    pc_menu_draw_centered(game, "...", PC_MENU_LIST_Y0 + l->rows * PC_MENU_LIST_LINE_H, 150, 150, 150, 200, 1.0f);
  }
}
