#include "pc_ap_critter_menu.h"
#include "pc_ap_critter_data.h"
#include "pc_ap_logic.h"
#include "pc_menu_list.h"
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

static int s_cat;   // mMmd_CATEGORY_INSECT / FISH
static int s_month; // 0-11
static int s_locked;
static pc_menu_list s_list;              // lines of s_month (none when locked)
static int s_species[PC_AP_CRITTER_NUM]; // museum index per line
static char s_names[PC_AP_CRITTER_NUM][mIN_ITEM_NAME_LEN + 1]; // museum index, game font, trailing spaces cut

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

static const pc_ap_critter_data* pc_ap_critter_menu_data(int idx) {
  return s_cat == mMmd_CATEGORY_FISH ? &pc_ap_fish_data[idx] : &pc_ap_bug_data[idx];
}

// Slots the player can use in s_month: all without Timesanity
static int pc_ap_critter_menu_reach_slots(void) {
  if(!pc_ap_time_frozen()) {
    return 0xF;
  }
  return (pc_ap_time_owned_months() & (1 << s_month)) ? pc_ap_time_owned_slots() : 0;
}

static void pc_ap_critter_menu_load(int month) {
  s_month = month;
  s_locked = pc_ap_time_frozen() && !(pc_ap_time_owned_months() & (1 << month));
  int count = 0;
  for(int idx = 0; idx < PC_AP_CRITTER_NUM && !s_locked; idx++) {
    const pc_ap_critter_data* d = pc_ap_critter_menu_data(idx);
    if(d->slots[month][0] | d->slots[month][1]) {
      s_species[count++] = idx;
    }
  }
  pc_menu_list_set(&s_list, count, 1, PC_AP_CRITTER_MENU_ROWS);
}

void pc_ap_critter_menu_enter(int cat) {
  s_cat = cat;
  for(int idx = 0; idx < PC_AP_CRITTER_NUM; idx++) {
    char* name = s_names[idx];
    mIN_copy_name_str((u8*)name, cat == mMmd_CATEGORY_FISH ? mNT_FishIdx2FishItemNo(idx) : ITM_INSECT_START + idx);
    int len = mIN_ITEM_NAME_LEN;
    while(len > 0 && name[len - 1] == CHAR_SPACE) {
      len--;
    }
    name[len] = 0;
  }
  pc_ap_critter_menu_load((Common_Get(time.rtc_time).month - 1) % 12); // lbRTC months are 1-12
}

void pc_ap_critter_menu_up(void) {
  pc_menu_list_up(&s_list);
}

void pc_ap_critter_menu_down(void) {
  pc_menu_list_down(&s_list);
}

void pc_ap_critter_menu_left(void) {
  pc_ap_critter_menu_load((s_month + 11) % 12);
}

void pc_ap_critter_menu_right(void) {
  pc_ap_critter_menu_load((s_month + 1) % 12);
}

// [star] name, slots, day range, weather; colour by status
static void pc_ap_critter_menu_line(struct game_s* game, int line, int column, f32 y) {
  f32 x_star = 14.0f;
  f32 x_name = 26.0f;
  f32 x_slot = 150.0f; // first slot column
  f32 slot_w = 20.0f;
  f32 x_range = 234.0f;
  f32 x_weather = 282.0f;
  f32 scale = 0.9f;
  int idx = s_species[line];
  const pc_ap_critter_data* d = pc_ap_critter_menu_data(idx);
  int r = 150, g = 150, b = 150, a = 200;
  int status = pc_ap_critter_status(s_cat, idx);
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
  pc_menu_draw_left(game, s_names[idx], x_name, y, r, g, b, a, scale);

  int half = pc_ap_critter_menu_half(d->slots[s_month], pc_ap_critter_menu_reach_slots());
  int slots = d->slots[s_month][half < 0 ? 0 : half];
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
      snprintf(text, sizeof(text), "16-%d", lbRTC_GetDaysByMonth(Common_Get(time.rtc_time).year, s_month + 1));
    }
    pc_menu_fixtext(text);
    pc_menu_draw_left(game, text, x_range, y, r, g, b, a, scale);
  }

  if(d->weather != PC_AP_WEATHER_ANY) {
    char sym[2] = { (char)(d->weather == PC_AP_WEATHER_RAIN_ONLY ? CHAR_SYMBOL_DROPLET : CHAR_SYMBOL_SUN), 0 };
    pc_menu_draw_left(game, sym, x_weather, y, r, g, b, a, scale);
  }
}

void pc_ap_critter_menu_draw(struct game_s* game) {
  char header[32];
  char status[32];
  snprintf(header, sizeof(header), "< %s >", l_month_names[s_month]);
  pc_menu_fixtext(header);
  if(s_locked) {
    pc_menu_list_draw(game, &s_list, header, "Locked", 1, pc_ap_critter_menu_line);
    return;
  }
  int has = 0;
  for(int i = 0; i < s_list.count; i++) {
    has += pc_ap_critter_status(s_cat, s_species[i]) > 0;
  }
  snprintf(status, sizeof(status), "%d / %d caught", has, s_list.count);
  pc_menu_fixtext(status);
  pc_menu_list_draw(game, &s_list, header, status, 0, pc_ap_critter_menu_line);
}
