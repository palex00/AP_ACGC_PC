// Pause menu Date & Time page: Year / Month / Day / Hour rows (left/right), then
// Change / Back. Hands the result to pc_ap_time_request; a new date asks first
// (the reload saves the game).
#ifndef PC_AP_TIME_MENU_H
#define PC_AP_TIME_MENU_H

#include "lb_rtc.h"
#include "pc_menu_util.h" // pc_menu_action, PC_MENU_STAY/BACK/RESUME

#ifdef __cplusplus
extern "C" {
#endif

struct game_s;

typedef struct {
  lbRTC_time_c now;     // clock when the page opened (minutes dropped)
  lbRTC_time_c pending; // date and hour being picked
  int sel;              // row
  int confirm;          // on the "the game will save" page
  int confirm_sel;      // 0 = No, 1 = Yes
} pc_ap_time_menu;

// Opens on the current date and hour
void pc_ap_time_menu_enter(pc_ap_time_menu* m);

// Returns PC_MENU_STAY / BACK / RESUME (change requested, or nothing to change)
int pc_ap_time_menu_input(pc_ap_time_menu* m, pc_menu_action act);

// Draws into NOW_FONT_DISP with its own dim backdrop. Font projection must be loaded.
void pc_ap_time_menu_draw(const pc_ap_time_menu* m, struct game_s* game);

#ifdef __cplusplus
}
#endif

#endif
