#include "pc_ap_time_menu.h"
#include "pc_ap_logic.h"
#include "pc_ap_timesanity.h" // slot_of_hour
#include "pc_menu_util.h"
#include "pc_text_draw.h"

#include "lb_rtc.h"
#include "m_font.h"
#include "m_kankyo.h"
#include "game.h"
#include "graph.h"

#include <stdio.h>

enum { ROW_YEAR, ROW_MONTH, ROW_DAY, ROW_HOUR, ROW_CHANGE, ROW_BACK, ROW_COUNT };

static int pc_ap_time_menu_same_date(const lbRTC_time_c* a, const lbRTC_time_c* b) {
  return lbRTC_IsEqualDate(a->year, a->month, a->day, b->year, b->month, b->day) == lbRTC_EQUAL;
}

void pc_ap_time_menu_enter(pc_ap_time_menu* m) {
  lbRTC_GetTime(&m->now);
  m->now.min = 0;
  m->now.sec = PC_AP_TIME_SEC;
  m->pending = m->now;
  m->sel = ROW_YEAR;
  m->confirm = 0;
}

void pc_ap_time_menu_up(pc_ap_time_menu* m) {
  if(m->confirm) {
    m->confirm_sel = !m->confirm_sel;
  } else {
    m->sel = (m->sel + ROW_COUNT - 1) % ROW_COUNT;
  }
}

void pc_ap_time_menu_down(pc_ap_time_menu* m) {
  if(m->confirm) {
    m->confirm_sel = !m->confirm_sel;
  } else {
    m->sel = (m->sel + 1) % ROW_COUNT;
  }
}

static void pc_ap_time_menu_step(pc_ap_time_menu* m, int dir) {
  if(m->confirm) {
    m->confirm_sel = !m->confirm_sel;
    return;
  }
  switch(m->sel) {
    case ROW_YEAR:  pc_ap_time_step_year(&m->pending, dir);  break;
    case ROW_MONTH: pc_ap_time_step_month(&m->pending, dir); break;
    case ROW_DAY:   pc_ap_time_step_day(&m->pending, dir);   break;
    case ROW_HOUR:  pc_ap_time_step_hour(&m->pending, dir);  break;
  }
}

void pc_ap_time_menu_left(pc_ap_time_menu* m) {
  pc_ap_time_menu_step(m, -1);
}

void pc_ap_time_menu_right(pc_ap_time_menu* m) {
  pc_ap_time_menu_step(m, 1);
}

int pc_ap_time_menu_confirm(pc_ap_time_menu* m) {
  if(m->confirm) {
    if(m->confirm_sel == 1) {
      pc_ap_time_request(&m->pending);
      return PC_AP_TIME_MENU_RESUME;
    }
    m->confirm = 0;
    return PC_AP_TIME_MENU_STAY;
  }
  if(m->sel == ROW_BACK) {
    return PC_AP_TIME_MENU_BACK;
  }
  if(m->sel != ROW_CHANGE) {
    m->sel = ROW_CHANGE;
    return PC_AP_TIME_MENU_STAY;
  }
  // New date: reload, which saves, so ask first. Same date: hour set live.
  if(!pc_ap_time_menu_same_date(&m->pending, &m->now)) {
    m->confirm = 1;
    m->confirm_sel = 0;
    return PC_AP_TIME_MENU_STAY;
  }
  if(m->pending.hour != m->now.hour) {
    pc_ap_time_request(&m->pending);
  }
  return PC_AP_TIME_MENU_RESUME;
}

int pc_ap_time_menu_cancel(pc_ap_time_menu* m) {
  if(m->confirm) {
    m->confirm = 0;
    return PC_AP_TIME_MENU_STAY;
  }
  return PC_AP_TIME_MENU_BACK;
}

static const char* l_month_names[12] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December",
};
static const char* l_weekday_names[lbRTC_WEEK] = {
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
};
static const char* l_slot_names[PC_AP_SLOT_NUM] = { "Morning", "Day", "Evening", "Night" };

static void pc_ap_time_menu_format(const pc_ap_time_menu* m, int row, char* out, int size) {
  const lbRTC_time_c* t = &m->pending;
  int hour12 = t->hour % 12 == 0 ? 12 : t->hour % 12;

  switch(row) {
    case ROW_YEAR:
      snprintf(out, size, "< %d >", t->year);
      break;
    case ROW_MONTH:
      snprintf(out, size, "< %s >", l_month_names[(t->month - 1) % 12]);
      break;
    case ROW_DAY:
      snprintf(out, size, "< %d >  %s", t->day, l_weekday_names[t->weekday % lbRTC_WEEK]);
      break;
    case ROW_HOUR:
      snprintf(out, size, "< %d %s >  %s", hour12, t->hour < 12 ? "AM" : "PM",
               l_slot_names[pc_ap_timesanity_slot_of_hour(t->hour)]);
      break;
    default:
      out[0] = 0;
      break;
  }
}

// Colored weather symbol of the picked date, after the Day row's text
static void pc_ap_time_menu_draw_weather(const pc_ap_time_menu* m, struct game_s* game, f32 vx, f32 y) {
  char value[64];
  char sym[2] = { 0, 0 };
  int r, g, b;
  int intensity;
  f32 s = m->sel == ROW_DAY ? PC_MENU_SCALE_SELECTED : 1.0f;
  f32 x;

  pc_ap_time_menu_format(m, ROW_DAY, value, sizeof(value));
  x = vx + (pc_text_width(value) + 6) * s;
  int weather = pc_ap_weather_of_date(&m->pending, &intensity);
  switch(weather) {
    case mEnv_WEATHER_RAIN:
      sym[0] = (char)CHAR_SYMBOL_DROPLET;
      r = 90, g = 160, b = 255;
      break;
    case mEnv_WEATHER_SNOW:
      sym[0] = (char)CHAR_SYMBOL_SNOWMAN;
      r = 255, g = 255, b = 255;
      break;
    default: // clear, cherry blossoms, falling leaves
      sym[0] = (char)CHAR_SYMBOL_SUN;
      r = 255, g = 210, b = 60;
      break;
  }
  pc_menu_draw_left(game, sym, x, y, r, g, b, 255, s);

  // Thunderstorm: lightning after the droplet
  if(weather == mEnv_WEATHER_RAIN && intensity == mEnv_WEATHER_INTENSITY_HEAVY) {
    x += pc_text_width(sym) * s;
    sym[0] = (char)CHAR_SYMBOL_LIGTNING;
    pc_menu_draw_left(game, sym, x, y, 255, 210, 60, 255, s);
  }
}

static void pc_ap_time_menu_draw_page(const pc_ap_time_menu* m, struct game_s* game) {
  static const char* labels[ROW_HOUR + 1] = { "Year", "Month", "Day", "Hour" };
  int r, g, b, a;
  f32 lx = 70.0f;
  f32 vx = 140.0f;
  f32 y0 = 70.0f;
  f32 line_h = 15.0f;
  char value[64];

  pc_menu_draw_centered(game, "- Date & Time -", 40.0f, 255, 255, 255, 255, 1.0f);
  for(int i = 0; i <= ROW_HOUR; i++) {
    int selected = m->sel == i;
    f32 s = selected ? PC_MENU_SCALE_SELECTED : 1.0f;
    pc_menu_row_colors(selected, &r, &g, &b, &a);
    pc_menu_draw_left(game, labels[i], lx, y0 + i * line_h, r, g, b, a, s);
    pc_ap_time_menu_format(m, i, value, sizeof(value));
    pc_menu_draw_left(game, value, vx, y0 + i * line_h, r, g, b, a, s);
  }

  pc_ap_time_menu_draw_weather(m, game, vx, y0 + ROW_DAY * line_h);

  f32 y = y0 + 4 * line_h + 10.0f;
  pc_menu_row_colors(m->sel == ROW_CHANGE, &r, &g, &b, &a);
  pc_menu_draw_centered(game, "Change", y, r, g, b, a, m->sel == ROW_CHANGE ? PC_MENU_SCALE_SELECTED : 1.0f);
  pc_menu_row_colors(m->sel == ROW_BACK, &r, &g, &b, &a);
  pc_menu_draw_centered(game, "Back", y + line_h, r, g, b, a, m->sel == ROW_BACK ? PC_MENU_SCALE_SELECTED : 1.0f);
}

static void pc_ap_time_menu_draw_confirm(const pc_ap_time_menu* m, struct game_s* game) {
  char date[64];

  snprintf(date, sizeof(date), "%s %d, %d", l_month_names[(m->pending.month - 1) % 12], m->pending.day,
           m->pending.year);
  pc_menu_draw_centered(game, "- Change the date? -", 80.0f, 255, 255, 255, 255, 1.0f);
  pc_menu_draw_centered(game, date, 105.0f, 230, 230, 230, 255, 1.0f);
  pc_menu_draw_centered(game, "The game will save.", 125.0f, 230, 230, 230, 255, 1.0f);
  pc_menu_draw_two_choice(game, "No", "Yes", m->confirm_sel, 160.0f);
}

void pc_ap_time_menu_draw(const pc_ap_time_menu* m, struct game_s* game) {
  pc_menu_dim_rect(game->graph, 180);
  if(m->confirm) {
    pc_ap_time_menu_draw_confirm(m, game);
  } else {
    pc_ap_time_menu_draw_page(m, game);
  }
}
