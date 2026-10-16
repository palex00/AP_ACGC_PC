// Sound test (--sound-test), see pc_sound_test.h
#include "pc_sound_test.h"
#include "pc_menu_util.h"
#include "pc_text_draw.h"

#include "audio.h"
#include "jaudio_NES/game64.h"
#include "m_controller.h"
#include "m_rcp.h"
#include "graph.h"
#include "dolphin/gx/GXFrameBuffer.h"

#include <stdio.h>

int g_pc_sound_test = 0;

#define PC_SOUND_TEST_ROWS        10
#define PC_SOUND_TEST_PAGE        16 // left / right move this many
#define PC_SOUND_TEST_REPEAT_WAIT 20 // frames held before repeating
#define PC_SOUND_TEST_REPEAT_STEP 3  // then one move every this many frames
#define PC_SOUND_TEST_STICK       40 // stick past this counts as up / down

static const char* l_tab_names[PC_SOUND_TEST_TAB_NUM] = { "Songs", "Loops", "Effects" };

// Ids listed per tab: first .. first + count - 1. Songs and loops are u8 ids, effects are a sound
// number (low 12 bits; the top 4 are play flags). Id 0 is "nothing" for loops and effects.
static int pc_sound_test_first(int tab) {
  return tab == PC_SOUND_TEST_SONGS ? 0 : 1;
}

static int pc_sound_test_count(int tab) {
  return tab == PC_SOUND_TEST_EFFECTS ? 0xFFF : (tab == PC_SOUND_TEST_SONGS ? 256 : 255);
}

// Enum entry for an id, NULL if it has none. Effects match on the sound number, so an entry with
// play flags (MONO(...), SE_ECHO(...)) names its plain number too.
static const pc_sound_test_name* pc_sound_test_find_name(int tab, int id) {
  if(tab == PC_SOUND_TEST_SONGS) {
    for(int i = 0; i < pc_sound_test_bgm_names_count; i++) {
      if(pc_sound_test_bgm_names[i].value == id) {
        return &pc_sound_test_bgm_names[i];
      }
    }
  } else if(tab == PC_SOUND_TEST_EFFECTS) {
    for(int i = 0; i < pc_sound_test_se_names_count; i++) {
      if((pc_sound_test_se_names[i].value & 0xFFF) == id) {
        return &pc_sound_test_se_names[i];
      }
    }
  }
  return NULL;
}

static void pc_sound_test_stop(GAME_SOUND_TEST* st) {
  sAdo_BgmStop(0);
  if(st->playing_tab == PC_SOUND_TEST_LOOPS) {
    sAdo_SysLevStop((u8)st->playing_id);
  }
  // Effects can't be stopped; they end by themselves
  st->playing_tab = -1;
}

static void pc_sound_test_play(GAME_SOUND_TEST* st) {
  int tab = st->tab;
  int id = pc_sound_test_first(tab) + st->sel[tab];

  pc_sound_test_stop(st);
  if(tab == PC_SOUND_TEST_SONGS) {
    sAdo_BgmStart((u8)id);
  } else if(tab == PC_SOUND_TEST_LOOPS) {
    sAdo_SysLevStart((u8)id);
  } else {
    // A named effect is played with its flags (MONO notes, echo, ...), as the game does
    const pc_sound_test_name* name = pc_sound_test_find_name(tab, id);
    if(name != NULL) {
      id = name->value;
    }
    sAdo_SysTrgStart((u16)id);
  }
  st->playing_tab = tab;
  st->playing_id = id;
  st->seconds = 0.0f;
  st->started = 0;
  st->ended_seconds = -1.0f;
}

static void pc_sound_test_move(GAME_SOUND_TEST* st, int delta) {
  int tab = st->tab;
  int sel = st->sel[tab] + delta;
  int count = pc_sound_test_count(tab);

  if(sel < 0) {
    sel = 0;
  }
  if(sel > count - 1) {
    sel = count - 1;
  }
  st->sel[tab] = sel;
  pc_menu_list_show(&st->list[tab], sel);
}

// Up / down (d-pad or stick) move by 1, left / right by a page; held = repeat
static int pc_sound_test_held_dir(void) {
  int stick_y = getJoystick_Y();

  if(chkButton(BUTTON_DUP) || stick_y > PC_SOUND_TEST_STICK) {
    return -1;
  }
  if(chkButton(BUTTON_DDOWN) || stick_y < -PC_SOUND_TEST_STICK) {
    return 1;
  }
  if(chkButton(BUTTON_DLEFT)) {
    return -PC_SOUND_TEST_PAGE;
  }
  if(chkButton(BUTTON_DRIGHT)) {
    return PC_SOUND_TEST_PAGE;
  }
  return 0;
}

static void pc_sound_test_input(GAME_SOUND_TEST* st) {
  int dir = pc_sound_test_held_dir();

  if(dir != st->hold_dir) {
    st->hold_dir = dir;
    st->hold_frames = 0;
    if(dir != 0) {
      pc_sound_test_move(st, dir);
    }
  } else if(dir != 0) {
    st->hold_frames++;
    if(st->hold_frames >= PC_SOUND_TEST_REPEAT_WAIT &&
       (st->hold_frames - PC_SOUND_TEST_REPEAT_WAIT) % PC_SOUND_TEST_REPEAT_STEP == 0) {
      pc_sound_test_move(st, dir);
    }
  }

  if(chkTrigger(BUTTON_L)) {
    st->tab = (st->tab + PC_SOUND_TEST_TAB_NUM - 1) % PC_SOUND_TEST_TAB_NUM;
  }
  if(chkTrigger(BUTTON_R)) {
    st->tab = (st->tab + 1) % PC_SOUND_TEST_TAB_NUM;
  }
  if(chkTrigger(BUTTON_A)) {
    pc_sound_test_play(st);
  }
  if(chkTrigger(BUTTON_B)) {
    pc_sound_test_stop(st);
  }
}

// Effects: the trigger slots tell whether it's still going (we only ever play one at a time)
static void pc_sound_test_update(GAME_SOUND_TEST* st, f32 dt) {
  if(st->playing_tab < 0) {
    return;
  }
  st->seconds += dt;
  if(st->playing_tab != PC_SOUND_TEST_EFFECTS || st->ended_seconds >= 0.0f) {
    return;
  }
  int busy;
  int sounding;
  Na_PcTrgStatus(&busy, &sounding);
  if(sounding > 0) {
    st->started = 1;
  }
  // Give the command a few frames to reach the engine before calling it done
  if(busy == 0 && st->seconds > 0.1f) {
    st->ended_seconds = st->seconds;
  }
}

static void pc_sound_test_status(const GAME_SOUND_TEST* st, char* out, size_t size) {
  if(!st->audio_ready) {
    snprintf(out, size, "Starting audio...");
  } else if(st->playing_tab < 0) {
    snprintf(out, size, "A play, B stop, L/R tab");
  } else if(st->playing_tab != PC_SOUND_TEST_EFFECTS) {
    snprintf(out, size, "Playing 0x%03X  %.1f s", st->playing_id, st->seconds);
  } else if(st->playing_id & 0x1000) {
    // MONO effects go to their own track, which the trigger slots don't track
    snprintf(out, size, "Played 0x%04X (mono, no status)", st->playing_id);
  } else if(st->ended_seconds < 0.0f) {
    snprintf(out, size, "Playing 0x%04X  %.1f s", st->playing_id, st->seconds);
  } else if(!st->started) {
    snprintf(out, size, "0x%04X never started", st->playing_id);
  } else {
    snprintf(out, size, "0x%04X ended after %.1f s", st->playing_id, st->ended_seconds);
  }
}

static void pc_sound_test_draw(GAME_SOUND_TEST* st) {
  GAME* game = &st->game;
  GRAPH* g = game->graph;
  int tab = st->tab;
  const pc_menu_list* list = &st->list[tab];
  char header[64];
  char text[64];

  // Background, as the model viewer does it
  GXColor clear = { 20, 20, 30, 255 };
  GXSetCopyClear(clear, 0x00FFFFFF);

  // Full-screen viewport: scenes set one each frame (showView), and the font list runs after the
  // opaque one in the same chain, so text (ours, the pause menu's, the overlay's) needs it too
  Vp* vp = GRAPH_ALLOC_TYPE(g, Vp, 1);
  vp->vp.vscale[0] = 640 * 2;
  vp->vp.vscale[1] = 480 * 2;
  vp->vp.vscale[2] = 511;
  vp->vp.vscale[3] = 0;
  vp->vp.vtrans[0] = 640 * 2;
  vp->vp.vtrans[1] = 480 * 2;
  vp->vp.vtrans[2] = 511;
  vp->vp.vtrans[3] = 0;

  OPEN_DISP(g);
  gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_0, 0);
  gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_0, 0);
  DisplayList_initialize(g, 20, 20, 30, NULL);
  gSPViewport(NOW_POLY_OPA_DISP++, vp);
  CLOSE_DISP(g);
  pc_menu_set_font_matrix(g);

  snprintf(header, sizeof(header), "Sound test: %s (%d / 3)", l_tab_names[tab], tab + 1);
  pc_menu_fixtext(header);
  pc_sound_test_status(st, text, sizeof(text));
  pc_menu_fixtext(text);
  pc_menu_list_draw(game, list, header, text, 0);

  for(int i = pc_menu_list_first(list); i < pc_menu_list_end(list); i++) {
    int id = pc_sound_test_first(tab) + i;
    const pc_sound_test_name* name = pc_sound_test_find_name(tab, id);
    int r, gr, b, a;
    pc_menu_row_colors(i == st->sel[tab], &r, &gr, &b, &a);
    snprintf(text, sizeof(text), "0x%03X  %s", id, name != NULL ? name->name : "");
    pc_menu_fixtext(text);
    pc_menu_draw_left(game, text, 60.0f, pc_menu_list_y(list, i), r, gr, b, a, 0.9f);
  }
}

static void pc_sound_test_main(GAME* game) {
  GAME_SOUND_TEST* st = (GAME_SOUND_TEST*)game;

  // The SE sequence starts when the scene mode leaves 0 (Na_SceneMode); 1 = field, as in game
  if(!st->audio_ready && Na_PcBooted()) {
    sAdo_SceneMode(1);
    st->audio_ready = 1;
  }
  if(st->audio_ready) {
    pc_sound_test_input(st);
  }
  pc_sound_test_update(st, (f32)game->graph->dt);
  pc_sound_test_draw(st);
}

void pc_sound_test_init(GAME* game) {
  GAME_SOUND_TEST* st = (GAME_SOUND_TEST*)game;

  game->exec = &pc_sound_test_main;
  game->cleanup = &pc_sound_test_cleanup;
  SetGameFrame(1);
  for(int tab = 0; tab < PC_SOUND_TEST_TAB_NUM; tab++) {
    pc_menu_list_set(&st->list[tab], pc_sound_test_count(tab), 1, PC_SOUND_TEST_ROWS);
    st->sel[tab] = 0;
  }
  st->tab = PC_SOUND_TEST_SONGS;
  st->playing_tab = -1;
}

void pc_sound_test_cleanup(GAME* game) {
  (void)game;
}
