#include "pc_ap_critter_menu.h"
#include "pc_ap_critter_data.h"
#include "pc_ap_logic.h"
#include "pc_menu_util.h"

#include "m_common_data.h"
#include "m_font.h"
#include "m_item_name.h"
#include "m_museum_display.h"
#include "m_name_table.h"
#include "lb_rtc.h"
#include "game.h"

#include <stdio.h>

#define PC_AP_CRITTER_MENU_ROWS 11 // visible lines

static const char* l_month_names[12] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December",
};
static const char* l_slot_names[4] = { "Mn", "Dy", "Ev", "Ni" };

int pc_ap_critter_menu_half(const u8 halves[2], int reach_slots) {
  if(halves[0] == halves[1]) {
    return -1;
  }
  if(halves[0] == 0 || halves[1] == 0) {
    return halves[0] == 0 ? 1 : 0;
  }
  int reach0 = (halves[0] & reach_slots) != 0;
  int reach1 = (halves[1] & reach_slots) != 0;
  return reach1 && !reach0 ? 1 : 0;
}

static const pc_ap_critter_data* pc_ap_critter_menu_data(const pc_ap_critter_menu* m, int idx) {
  return m->cat == mMmd_CATEGORY_FISH ? &pc_ap_fish_data[idx] : &pc_ap_bug_data[idx];
}

// Slots the player can use in month: all without Timesanity
static int pc_ap_critter_menu_reach_slots(int month) {
  if(!pc_ap_time_frozen()) {
    return 0xF;
  }
  return (pc_ap_time_owned_months() & (1 << month)) ? pc_ap_time_owned_slots() : 0;
}

static void pc_ap_critter_menu_load(pc_ap_critter_menu* m, int month) {
  m->month = month;
  m->locked = pc_ap_time_frozen() && !(pc_ap_time_owned_months() & (1 << month));
  int count = 0;
  for(int idx = 0; idx < PC_AP_CRITTER_NUM && !m->locked; idx++) {
    const pc_ap_critter_data* d = pc_ap_critter_menu_data(m, idx);
    if(d->slots[month][0] | d->slots[month][1]) {
      m->species[count++] = idx;
    }
  }
  pc_menu_list_set(&m->list, count, 1, PC_AP_CRITTER_MENU_ROWS);
}

void pc_ap_critter_menu_enter(pc_ap_critter_menu* m, int cat) {
  m->cat = cat;
  for(int idx = 0; idx < PC_AP_CRITTER_NUM; idx++) {
    char* name = m->names[idx];
    mIN_copy_name_str((u8*)name, cat == mMmd_CATEGORY_FISH ? mNT_FishIdx2FishItemNo(idx) : ITM_INSECT_START + idx);
    int len = mIN_ITEM_NAME_LEN;
    while(len > 0 && name[len - 1] == CHAR_SPACE) {
      len--;
    }
    name[len] = 0;
  }
  pc_ap_critter_menu_load(m, (Common_Get(time.rtc_time).month - 1) % 12); // lbRTC months are 1-12
}

void pc_ap_critter_menu_up(pc_ap_critter_menu* m) {
  pc_menu_list_up(&m->list);
}

void pc_ap_critter_menu_down(pc_ap_critter_menu* m) {
  pc_menu_list_down(&m->list);
}

void pc_ap_critter_menu_left(pc_ap_critter_menu* m) {
  pc_ap_critter_menu_load(m, (m->month + 11) % 12);
}

void pc_ap_critter_menu_right(pc_ap_critter_menu* m) {
  pc_ap_critter_menu_load(m, (m->month + 1) % 12);
}

// [star] name, slots, day range, weather; colour by status
static void pc_ap_critter_menu_line(const pc_ap_critter_menu* m, struct game_s* game, int line) {
  f32 x_star = 14.0f;
  f32 x_name = 26.0f;
  f32 x_slot = 150.0f; // first slot column
  f32 slot_w = 20.0f;
  f32 x_range = 234.0f;
  f32 x_weather = 282.0f;
  f32 scale = 0.9f;
  f32 y = pc_menu_list_y(&m->list, line);
  int idx = m->species[line];
  const pc_ap_critter_data* d = pc_ap_critter_menu_data(m, idx);
  int r = 150, g = 150, b = 150, a = 200;
  int status = pc_ap_critter_status(m->cat, idx);
  if(status == 1) {
    r = g = b = a = 255;
  } else if(status >= 2) {
    r = 255, g = 215, b = 0, a = 255;
  }
  char text[16];

  if(status > 0) {
    char star[2] = { (char)CHAR_SYMBOL_STAR, 0 };
    pc_menu_draw_left(game, star, x_star, y, r, g, b, a, scale);
  }
  pc_menu_draw_left(game, m->names[idx], x_name, y, r, g, b, a, scale);

  int half = pc_ap_critter_menu_half(d->slots[m->month], pc_ap_critter_menu_reach_slots(m->month));
  int slots = d->slots[m->month][half < 0 ? 0 : half];
  for(int s = 0; s < 4; s++) {
    if(slots & (1 << s)) {
      snprintf(text, sizeof(text), "%s", l_slot_names[s]);
      pc_menu_fixtext(text);
      pc_menu_draw_left(game, text, x_slot + s * slot_w, y, r, g, b, a, scale);
    }
  }

  if(half >= 0) {
    if(half == 0) {
      snprintf(text, sizeof(text), "1-15");
    } else {
      snprintf(text, sizeof(text), "16-%d", lbRTC_GetDaysByMonth(Common_Get(time.rtc_time).year, m->month + 1));
    }
    pc_menu_fixtext(text);
    pc_menu_draw_left(game, text, x_range, y, r, g, b, a, scale);
  }

  if(d->weather != PC_AP_WEATHER_ANY) {
    char sym[2] = { (char)(d->weather == PC_AP_WEATHER_RAIN_ONLY ? CHAR_SYMBOL_DROPLET : CHAR_SYMBOL_SUN), 0 };
    pc_menu_draw_left(game, sym, x_weather, y, r, g, b, a, scale);
  }
}

void pc_ap_critter_menu_draw(const pc_ap_critter_menu* m, struct game_s* game) {
  char header[32];
  char status[32];
  snprintf(header, sizeof(header), "< %s >", l_month_names[m->month]);
  pc_menu_fixtext(header);
  if(m->locked) {
    pc_menu_list_draw(game, &m->list, header, "Locked", 1);
    return;
  }
  int has = 0;
  for(int i = 0; i < m->list.count; i++) {
    has += pc_ap_critter_status(m->cat, m->species[i]) > 0;
  }
  snprintf(status, sizeof(status), "%d / %d caught", has, m->list.count);
  pc_menu_fixtext(status);
  pc_menu_list_draw(game, &m->list, header, status, 0);
  for(int i = pc_menu_list_first(&m->list); i < pc_menu_list_end(&m->list); i++) {
    pc_ap_critter_menu_line(m, game, i);
  }
}
