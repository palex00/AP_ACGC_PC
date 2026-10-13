#include "pc_ap_catalog_menu.h"
#include "pc_ap_logic.h"
#include "pc_menu_list.h"
#include "pc_menu_util.h"
#include "ap_slotdata.h" // AP_CATALOG_MAX

#include "m_font.h"
#include "m_item_name.h"
#include "m_shop.h" // mSP_CollectCheck
#include "game.h"
#include "graph.h"

#include <stdio.h>

#define PC_AP_CATALOG_MENU_ROWS 11 // visible rows of two items

static int s_pool; // 0-based
static pc_menu_list s_list; // items of s_pool (none when locked)
static mActor_name_t s_items[AP_CATALOG_MAX];
static char s_names[AP_CATALOG_MAX][mIN_ITEM_NAME_LEN + 1]; // game font, trailing spaces cut

static void pc_ap_catalog_menu_load(int pool) {
  s_pool = pool;
  int count = pc_ap_catalog_pool_items(pool, s_items, AP_CATALOG_MAX);
  pc_menu_list_set(&s_list, count, 2, PC_AP_CATALOG_MENU_ROWS);
  for(int i = 0; i < count; i++) {
    char* name = s_names[i];
    mIN_copy_name_str((u8*)name, s_items[i]);
    int len = mIN_ITEM_NAME_LEN;
    while(len > 0 && name[len - 1] == CHAR_SPACE) {
      len--;
    }
    name[len] = 0;
  }
}

void pc_ap_catalog_menu_enter(void) {
  pc_ap_catalog_menu_load(pc_ap_catalog_first_unfinished_pool());
}

void pc_ap_catalog_menu_up(void) {
  pc_menu_list_up(&s_list);
}

void pc_ap_catalog_menu_down(void) {
  pc_menu_list_down(&s_list);
}

// Next/previous pool, wrapping around
static void pc_ap_catalog_menu_step(int dir) {
  int total = pc_ap_catalog_pools_total();
  if(total > 0) {
    pc_ap_catalog_menu_load((s_pool + dir + total) % total);
  }
}

void pc_ap_catalog_menu_left(void) {
  pc_ap_catalog_menu_step(-1);
}

void pc_ap_catalog_menu_right(void) {
  pc_ap_catalog_menu_step(1);
}

// Star + white when in the catalog, else gray
static void pc_ap_catalog_menu_item(struct game_s* game, int i, int column, f32 y) {
  f32 x = column == 0 ? 18.0f : 163.0f;
  f32 star_w = 12.0f;
  f32 scale = 0.9f;
  if(mSP_CollectCheck(s_items[i])) {
    char star[2] = { (char)CHAR_SYMBOL_STAR, 0 };
    pc_menu_draw_left(game, star, x, y, 255, 255, 255, 255, scale);
    pc_menu_draw_left(game, s_names[i], x + star_w, y, 255, 255, 255, 255, scale);
  } else {
    pc_menu_draw_left(game, s_names[i], x + star_w, y, 150, 150, 150, 200, scale);
  }
}

void pc_ap_catalog_menu_draw(struct game_s* game) {
  char header[64];
  char status[64];
  snprintf(header, sizeof(header), "< Catalog Pool %d / %d >", s_pool + 1, pc_ap_catalog_pools_total());
  pc_menu_fixtext(header);

  if(s_pool >= pc_ap_catalog_pools_open()) {
    pc_menu_list_draw(game, &s_list, header, "Locked", 1, pc_ap_catalog_menu_item);
    return;
  }
  int done = 0;
  for(int i = 0; i < s_list.count; i++) {
    done += mSP_CollectCheck(s_items[i]) ? 1 : 0;
  }
  snprintf(status, sizeof(status), "%d / %d collected", done, s_list.count);
  pc_menu_fixtext(status);
  pc_menu_list_draw(game, &s_list, header, status, 0, pc_ap_catalog_menu_item);
}
