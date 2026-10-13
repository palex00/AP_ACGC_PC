#include "pc_ap_catalog_menu.h"
#include "pc_ap_logic.h"
#include "pc_menu_util.h"
#include "ap_slotdata.h" // AP_CATALOG_MAX

#include "m_font.h"
#include "m_item_name.h"
#include "m_shop.h" // mSP_CollectCheck
#include "game.h"
#include "graph.h"

#include <stdio.h>

#define PC_AP_CATALOG_MENU_ROWS 11 // visible rows of two items

static int s_pool;   // 0-based
static int s_scroll; // first visible row
static int s_count;  // items of s_pool (0 when locked)
static mActor_name_t s_items[AP_CATALOG_MAX];
static char s_names[AP_CATALOG_MAX][mIN_ITEM_NAME_LEN + 1]; // game font, trailing spaces cut

static void pc_ap_catalog_menu_load(int pool) {
  s_pool = pool;
  s_scroll = 0;
  s_count = pc_ap_catalog_pool_items(pool, s_items, AP_CATALOG_MAX);
  for(int i = 0; i < s_count; i++) {
    char* name = s_names[i];
    mIN_copy_name_str((u8*)name, s_items[i]);
    int len = mIN_ITEM_NAME_LEN;
    while(len > 0 && name[len - 1] == CHAR_SPACE) {
      len--;
    }
    name[len] = 0;
  }
}

static int pc_ap_catalog_menu_max_scroll(void) {
  int rows = (s_count + 1) / 2;
  return rows > PC_AP_CATALOG_MENU_ROWS ? rows - PC_AP_CATALOG_MENU_ROWS : 0;
}

void pc_ap_catalog_menu_enter(void) {
  pc_ap_catalog_menu_load(pc_ap_catalog_first_unfinished_pool());
}

void pc_ap_catalog_menu_up(void) {
  if(s_scroll > 0) {
    s_scroll--;
  }
}

void pc_ap_catalog_menu_down(void) {
  if(s_scroll < pc_ap_catalog_menu_max_scroll()) {
    s_scroll++;
  }
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

void pc_ap_catalog_menu_draw(struct game_s* game) {
  f32 x_col[2] = { 18.0f, 163.0f };
  f32 star_w = 12.0f;
  f32 y0 = 66.0f;
  f32 line_h = 14.0f;
  f32 list_scale = 0.9f;
  char text[64];

  pc_menu_dim_rect(game->graph, 180);
  snprintf(text, sizeof(text), "< Catalog Pool %d / %d >", s_pool + 1, pc_ap_catalog_pools_total());
  pc_menu_fixtext(text);
  pc_menu_draw_centered(game, text, 22.0f, 255, 255, 255, 255, 1.0f);

  if(s_pool >= pc_ap_catalog_pools_open()) {
    pc_menu_draw_centered(game, "Locked", 38.0f, 150, 150, 150, 220, 1.0f);
    return;
  }
  int done = 0;
  for(int i = 0; i < s_count; i++) {
    done += mSP_CollectCheck(s_items[i]) ? 1 : 0;
  }
  snprintf(text, sizeof(text), "%d / %d collected", done, s_count);
  pc_menu_fixtext(text);
  pc_menu_draw_centered(game, text, 38.0f, 230, 230, 230, 255, 1.0f);

  int first = s_scroll * 2;
  for(int i = first; i < s_count && i < first + PC_AP_CATALOG_MENU_ROWS * 2; i++) {
    f32 x = x_col[i % 2];
    f32 y = y0 + ((i - first) / 2) * line_h;
    if(mSP_CollectCheck(s_items[i])) {
      char star[2] = { (char)CHAR_SYMBOL_STAR, 0 };
      pc_menu_draw_left(game, star, x, y, 255, 255, 255, 255, list_scale);
      pc_menu_draw_left(game, s_names[i], x + star_w, y, 255, 255, 255, 255, list_scale);
    } else {
      pc_menu_draw_left(game, s_names[i], x + star_w, y, 150, 150, 150, 200, list_scale);
    }
  }

  // More above / below
  if(s_scroll > 0) {
    pc_menu_draw_centered(game, "...", y0 - 12.0f, 150, 150, 150, 200, 1.0f);
  }
  if(s_scroll < pc_ap_catalog_menu_max_scroll()) {
    pc_menu_draw_centered(game, "...", y0 + PC_AP_CATALOG_MENU_ROWS * line_h, 150, 150, 150, 200, 1.0f);
  }
}
