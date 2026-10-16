// Sound test (--sound-test): the game boots straight into this instead of the title, so nothing else
// plays. Three tabs: Songs (sAdo_BgmStart ids), Loops (sAdo_SysLevStart ids), Effects
// (sAdo_SysTrgStart ids). Names come from the enums in audio_defs.h where an id has one.
// Removing it: this file, pc_sound_test.c, pc_sound_test_names.c, and the TARGET_PC lines tagged
// "sound test" (pc_main.c, graph.c, game.c, m_game_dlftbls.c, game64.c_inc / game64.h).
#ifndef PC_SOUND_TEST_H
#define PC_SOUND_TEST_H

#include "game.h"
#include "pc_menu_list.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  PC_SOUND_TEST_SONGS,
  PC_SOUND_TEST_LOOPS,
  PC_SOUND_TEST_EFFECTS,
  PC_SOUND_TEST_TAB_NUM
};

extern int g_pc_sound_test;

// Generated (pc_sound_test_names.c): every NA_SE_* / BGM_* enum entry
typedef struct {
  int value;
  const char* name;
} pc_sound_test_name;
extern const pc_sound_test_name pc_sound_test_se_names[];
extern const int pc_sound_test_se_names_count;
extern const pc_sound_test_name pc_sound_test_bgm_names[];
extern const int pc_sound_test_bgm_names_count;

// Must fit inside alloc_size declared in game_dlftbls
typedef struct {
  GAME game; // must be first (base class)
  int tab;
  int sel[PC_SOUND_TEST_TAB_NUM];
  pc_menu_list list[PC_SOUND_TEST_TAB_NUM];
  int hold_dir;    // -1 / 1 while up or down (or left / right x 16) is held, else 0
  int hold_frames; // frames it has been held, for key repeat
  int audio_ready; // scene mode set once the engine booted (starts the SE sequence)
  // The last thing played
  int playing_tab;   // -1 = nothing
  int playing_id;    // id as sent to the engine
  f32 seconds;       // since it was played
  int started;       // Effects: the SE sequence reported it sounding at some point
  f32 ended_seconds; // Effects: how long it lasted, -1 while still going
} GAME_SOUND_TEST;

void pc_sound_test_init(GAME* game);
void pc_sound_test_cleanup(GAME* game);

#ifdef __cplusplus
}
#endif

#endif
