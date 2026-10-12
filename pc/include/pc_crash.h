#ifndef PC_CRASH_H
#define PC_CRASH_H

// Crash log: on an unhandled exception, writes crash_<date>_<time>.txt next to the exe
// (exception, registers, stack scan, last game state, loaded modules, AP log tail) and
// shows an error box. Decode with ignore/tools/crash_decode.sh. Windows only; no-ops elsewhere.

struct game_play_s;

// Installs the handler. First thing in main.
void pc_crash_init(void);
// Records OS and GL info for the report. After the GL context exists.
void pc_crash_init_info(void);
// Per-frame snapshot of game state (scene, date, weather, player, heap). From pc_ap_tick.
void pc_crash_note_frame(struct game_play_s* play);
// --crash-test: crashes on purpose to check the report
void pc_crash_test(void);

#endif
