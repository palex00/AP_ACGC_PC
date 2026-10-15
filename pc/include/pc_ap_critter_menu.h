// Pause menu Bugs and Fish pages: one month at a time (left/right, wraps; opens on
// the current month), the species catchable that month in museum order, one per line:
// [star] name, time slots (Mn Dy Ev Ni), day range, weather symbol. Gray = has none,
// white + star = has one, gold = has both (pc_ap_critter_status). A month not owned
// under Timesanity shows only "Locked". Up/down scrolls by line.
// Fish with Fish Availability "Progressive": the fish are split into one section per
// fish pass (header line "= Small River Fish =", ...); a closed pass shows one
// "(Locked)" line instead of its fish; a pass with no fish that month has no section;
// a fish of several passes is listed under each. "x / y caught" counts the whole month.
// Bugs with Bug Availability "Progressive": the same with bug passes ("= Tree Bugs =", ...).
#ifndef PC_AP_CRITTER_MENU_H
#define PC_AP_CRITTER_MENU_H

#include "types.h"
#include "m_item_name.h"
#include "pc_ap_museumsanity.h" // PC_AP_CRITTER_NUM
#include "pc_menu_list.h"
#include "pc_menu_util.h" // pc_menu_action

#ifdef __cplusplus
extern "C" {
#endif

struct game_s;

// Line kinds
#define PC_AP_CRITTER_LINE_SPECIES 0 // value = museum index
#define PC_AP_CRITTER_LINE_HEADER  1 // value = fish or bug pass (PC_AP_FISH_PASS_* / PC_AP_BUG_PASS_*)
#define PC_AP_CRITTER_LINE_LOCKED  2 // value = pass
// Fish of several passes twice, plus a header and a "(Locked)" per pass
#define PC_AP_CRITTER_LINES_MAX (2 * PC_AP_CRITTER_NUM + 2 * PC_AP_FISH_PASS_NUM)

typedef struct {
  u8 kind;  // PC_AP_CRITTER_LINE_*
  u8 value;
} pc_ap_critter_line;

typedef struct {
  int cat;   // mMmd_CATEGORY_INSECT / FISH
  int month; // 0-11
  int locked;
  pc_menu_list list;                             // lines of month (none when locked)
  pc_ap_critter_line lines[PC_AP_CRITTER_LINES_MAX];
  int month_count;                               // species of month (each once)
  int month_species[PC_AP_CRITTER_NUM];          // their museum indices
  char names[PC_AP_CRITTER_NUM][mIN_ITEM_NAME_LEN + 1]; // by museum index, game font, trailing spaces cut
} pc_ap_critter_menu;

// cat: mMmd_CATEGORY_INSECT or mMmd_CATEGORY_FISH. Needs a player.
void pc_ap_critter_menu_enter(pc_ap_critter_menu* m, int cat);

// Up/down scroll, left/right page, cancel = PC_MENU_BACK; else PC_MENU_STAY
int pc_ap_critter_menu_input(pc_ap_critter_menu* m, pc_menu_action act);

// Draws into NOW_FONT_DISP with its own dim backdrop. Font projection must be loaded.
void pc_ap_critter_menu_draw(const pc_ap_critter_menu* m, struct game_s* game);

// Which half of the month a line shows, from its slot bits per half (days 1-15, 16-end):
// -1 = halves the same (no day range); a half with no slots -> the other one; both have
// different slots -> the one reachable with reach_slots (slots the player can use this
// month), both or neither -> 0.
int pc_ap_critter_menu_half(const u8 halves[2], int reach_slots);

#ifdef __cplusplus
}
#endif

#endif
