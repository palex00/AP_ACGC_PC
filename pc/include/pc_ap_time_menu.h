// Pause menu Date & Time page: Year / Month / Day / Hour rows (left/right), then
// Change / Back. Hands the result to pc_ap_time_request; a new date asks first
// (the reload saves the game).
#ifndef PC_AP_TIME_MENU_H
#define PC_AP_TIME_MENU_H

#include "lb_rtc.h"

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

// What the pause menu should do after an input
enum {
  PC_AP_TIME_MENU_STAY,   // page stays open
  PC_AP_TIME_MENU_BACK,   // back to the pause menu's main page
  PC_AP_TIME_MENU_RESUME, // close the pause menu (change requested, or nothing to change)
};

// Opens on the current date and hour
void pc_ap_time_menu_enter(pc_ap_time_menu* m);

void pc_ap_time_menu_up(pc_ap_time_menu* m);
void pc_ap_time_menu_down(pc_ap_time_menu* m);
void pc_ap_time_menu_left(pc_ap_time_menu* m);
void pc_ap_time_menu_right(pc_ap_time_menu* m);
int  pc_ap_time_menu_confirm(pc_ap_time_menu* m); // PC_AP_TIME_MENU_*
int  pc_ap_time_menu_cancel(pc_ap_time_menu* m);  // PC_AP_TIME_MENU_*

// Draws into NOW_FONT_DISP with its own dim backdrop. Font projection must be loaded.
void pc_ap_time_menu_draw(const pc_ap_time_menu* m, struct game_s* game);

#ifdef __cplusplus
}
#endif

#endif
