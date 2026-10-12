#include "pc_platform.h"
#include "pc_crash.h"
#include "ap_archipelago.h"
#include "ap_log.h"

#include "m_common_data.h"
#include "m_play.h"
#include "m_player_lib.h"
#include "m_malloc.h"
#include "m_kankyo.h"
#include "game.h"

#ifdef _WIN32
#include <tlhelp32.h>

#define CRASH_SCENES 16    // scene changes kept
#define CRASH_STACK_BYTES 0x4000
#define CRASH_STACK_HITS 64
#define CRASH_MODULES 128
#define CRASH_LOG_LINES 40

// Filled during normal play, so the handler only prints memory it owns
typedef struct {
  int frame; // pc_crash_note_frame calls (play frames only)
  DWORD tick_ms; // GetTickCount at the last snapshot
  int scene;
  lbRTC_time_c date;
  int weather;
  int weather_intensity;
  int has_player;
  int player_x, player_y, player_z;
  int ap_state;
} CrashFrame;

typedef struct {
  int frame;
  DWORD tick_ms;
  int from, to;
  size_t heap_free, heap_max_block, heap_alloc;
} CrashScene;

typedef struct {
  char name[64];
  DWORD base, size;
} CrashModule;

static char s_os[128] = "?";
static char s_gl[384] = "?";
static CrashFrame s_frame;
static CrashScene s_scenes[CRASH_SCENES];
static int s_scene_count; // total ever, ring index = count % CRASH_SCENES
static int s_last_scene = -1;
static DWORD s_start_ms;

// Handler state: static so a stack overflow has little left to use
static volatile LONG s_in_handler;
static HANDLE s_file = INVALID_HANDLE_VALUE;
static char s_line[1024];
// Two lists: taken at startup, and again at crash time. The crash-time one is used if it
// works (it can't if the crash broke the heap), so DLLs loaded later show up too.
static CrashModule s_modules_init[CRASH_MODULES];
static CrashModule s_modules_crash[CRASH_MODULES];
static int s_module_count_init;
static CrashModule* s_modules = s_modules_init;
static int s_module_count;
static struct ApLogEntry s_log[CRASH_LOG_LINES];

static void crash_write(const char* fmt, ...) {
  va_list args;
  DWORD written;
  va_start(args, fmt);
  int n = wvsprintfA(s_line, fmt, args); // no floats, no malloc; caps at 1024
  va_end(args);
  if(n > 0 && s_file != INVALID_HANDLE_VALUE) {
    WriteFile(s_file, s_line, (DWORD)n, &written, NULL);
  }
}

static void crash_flush(void) {
  if(s_file != INVALID_HANDLE_VALUE) {
    FlushFileBuffers(s_file);
  }
}

// Reads that fail instead of faulting
static int crash_read(const void* addr, void* out, SIZE_T size) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(), addr, out, size, &got) && got == size;
}

static int crash_collect_modules(CrashModule* out) {
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
  MODULEENTRY32 me;
  int count = 0;

  if(snap == INVALID_HANDLE_VALUE) {
    return 0;
  }
  me.dwSize = sizeof(me);
  if(Module32First(snap, &me)) {
    do {
      CrashModule* m = &out[count++];
      lstrcpynA(m->name, me.szModule, sizeof(m->name));
      m->base = (DWORD)(uintptr_t)me.modBaseAddr;
      m->size = me.modBaseSize;
    } while(count < CRASH_MODULES && Module32Next(snap, &me));
  }
  CloseHandle(snap);
  return count;
}

static const CrashModule* crash_module_of(DWORD addr) {
  for(int i = 0; i < s_module_count; i++) {
    if(addr - s_modules[i].base < s_modules[i].size) {
      return &s_modules[i];
    }
  }
  return NULL;
}

// "module+0xoffset", or the bare address
static void crash_format_addr(char* out, DWORD addr) {
  const CrashModule* m = crash_module_of(addr);
  if(m != NULL) {
    wsprintfA(out, "%s+0x%lx", m->name, addr - m->base);
  } else {
    wsprintfA(out, "0x%08lx", addr);
  }
}

// Likely a return address: the bytes before it are a call (E8 rel32, or FF /2 in its
// 2/3/6/7-byte forms). Cuts most of the stale data out of the stack scan.
static int crash_after_call(DWORD addr) {
  unsigned char b[7];
  if(crash_module_of(addr) == NULL || !crash_read((const void*)(uintptr_t)(addr - 7), b, sizeof(b))) {
    return 0;
  }
  if(b[2] == 0xE8) {
    return 1;
  }
  static const int back[] = { 2, 3, 6, 7 };
  for(int i = 0; i < 4; i++) {
    int at = 7 - back[i];
    if(b[at] == 0xFF && ((b[at + 1] >> 3) & 7) == 2) {
      return 1;
    }
  }
  return 0;
}

static const char* crash_code_name(DWORD code) {
  switch(code) {
    case EXCEPTION_ACCESS_VIOLATION: return "access violation";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "misaligned data";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "float divide by zero";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
    case EXCEPTION_IN_PAGE_ERROR: return "in-page error";
    case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
    case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
    case 0xC0000409: return "stack buffer overrun / fail-fast";
    case 0xC0000374: return "heap corruption";
    default: return "unknown";
  }
}

static void crash_write_exception(EXCEPTION_POINTERS* ep) {
  EXCEPTION_RECORD* er = ep->ExceptionRecord;
  CONTEXT* c = ep->ContextRecord;
  char where[96];

  crash_format_addr(where, (DWORD)(uintptr_t)er->ExceptionAddress);
  crash_write("[exception]\r\ncode 0x%08lx (%s) at %s\r\n", er->ExceptionCode,
              crash_code_name(er->ExceptionCode), where);
  if(er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2) {
    static const char* kinds[] = { "reading", "writing", "?", "?", "?", "?", "?", "?", "executing" };
    ULONG_PTR kind = er->ExceptionInformation[0];
    crash_write("%s 0x%08lx\r\n", kind <= 8 ? kinds[kind] : "?", (DWORD)er->ExceptionInformation[1]);
  }
  crash_write("thread %lu\r\n", GetCurrentThreadId());
  crash_write("\r\n[registers]\r\neip %08lx esp %08lx ebp %08lx eflags %08lx\r\n", c->Eip, c->Esp, c->Ebp,
              c->EFlags);
  crash_write("eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx\r\n", c->Eax, c->Ebx, c->Ecx,
              c->Edx, c->Esi, c->Edi);
  crash_flush();
}

static void crash_write_stack(CONTEXT* c) {
  DWORD top = c->Esp;
  DWORD end = top + CRASH_STACK_BYTES; // the scan also stops at the stack's top (first failed read)
  char where[96];
  int hits = 0;

  crash_write("\r\n[stack scan] return-address-like values from esp up (newest first)\r\n");
  crash_format_addr(where, c->Eip);
  crash_write("  eip      %s\r\n", where);
  for(DWORD p = top & ~3u; p + 4 <= end && hits < CRASH_STACK_HITS; p += 4) {
    DWORD v;
    if(!crash_read((const void*)(uintptr_t)p, &v, sizeof(v))) {
      break;
    }
    if(crash_after_call(v)) {
      crash_format_addr(where, v);
      crash_write("  esp+%04lx %s\r\n", p - top, where);
      hits++;
    }
  }
  crash_flush();
}

static void crash_write_state(void) {
  CrashFrame f = s_frame;
  GAME* game = NULL;
  unsigned char points[2];
  DWORD now = GetTickCount();

  crash_write("\r\n[game]\r\nuptime %lu s\r\n", (now - s_start_ms) / 1000);
  if(crash_read(&gamePT, &game, sizeof(game)) && game != NULL &&
     crash_read(&game->doing_point, points, sizeof(points))) {
    crash_write("doing_point %d, doing_point_specific 0x%02x\r\n", points[0], points[1]);
  }
  if(f.frame == 0) {
    crash_write("no play frame yet\r\n");
  } else {
    crash_write("last play frame %d, %lu ms before the crash\r\n", f.frame, now - f.tick_ms);
    crash_write("scene %d\r\n", f.scene);
    crash_write("date %d-%02d-%02d %02d:%02d:%02d\r\n", f.date.year, f.date.month, f.date.day, f.date.hour,
                f.date.min, f.date.sec);
    crash_write("weather %d intensity %d\r\n", f.weather, f.weather_intensity);
    if(f.has_player) {
      crash_write("player %d %d %d\r\n", f.player_x, f.player_y, f.player_z);
    } else {
      crash_write("player none\r\n");
    }
    crash_write("ap state %d\r\n", f.ap_state);
  }

  crash_write("\r\n[scene changes] oldest first: frame, ms since start, from -> to, heap free/max block/allocated\r\n");
  int first = s_scene_count > CRASH_SCENES ? s_scene_count - CRASH_SCENES : 0;
  for(int i = first; i < s_scene_count; i++) {
    const CrashScene* s = &s_scenes[i % CRASH_SCENES];
    crash_write("  %d %lu %d -> %d  %lu/%lu/%lu\r\n", s->frame, s->tick_ms - s_start_ms, s->from, s->to,
                (DWORD)s->heap_free, (DWORD)s->heap_max_block, (DWORD)s->heap_alloc);
  }
  crash_flush();
}

static void crash_write_modules(void) {
  crash_write("\r\n[modules]\r\n");
  for(int i = 0; i < s_module_count; i++) {
    crash_write("  %08lx %08lx %s\r\n", s_modules[i].base, s_modules[i].size, s_modules[i].name);
  }
  crash_flush();
}

static void crash_write_log(void) {
  size_t n = ap_log_try_get(CRASH_LOG_LINES, s_log);
  crash_write("\r\n[ap log] newest first%s\r\n", n == 0 ? " (empty or locked)" : "");
  for(size_t i = 0; i < n; i++) {
    crash_write("  %s\r\n", s_log[i].msg);
  }
  crash_flush();
}

static LONG WINAPI pc_crash_handler(EXCEPTION_POINTERS* ep) {
  char name[64];
  char msg[256];
  SYSTEMTIME t;

  // A crash inside the report: give up quietly
  if(InterlockedExchange(&s_in_handler, 1) != 0) {
    return EXCEPTION_EXECUTE_HANDLER;
  }
  GetLocalTime(&t);
  wsprintfA(name, "crash_%04d%02d%02d_%02d%02d%02d.txt", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
            t.wSecond);
  s_file = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  // Most useful first, each section flushed, so a failure late still leaves the rest
  crash_write("Animal Crossing AP crash report\r\nversion %s\r\nos %s\r\ngl %s\r\n\r\n", PC_AP_VERSION, s_os,
              s_gl);
  s_module_count = s_module_count_init;
  int n = crash_collect_modules(s_modules_crash);
  if(n > 0) {
    s_modules = s_modules_crash;
    s_module_count = n;
  }
  crash_write_exception(ep);
  crash_write_stack(ep->ContextRecord);
  crash_write_state();
  crash_write_modules();
  crash_write_log();
  if(s_file != INVALID_HANDLE_VALUE) {
    CloseHandle(s_file);
  }

  if(s_file != INVALID_HANDLE_VALUE) {
    wsprintfA(msg,
              "The game crashed.\n\nDetails were saved to %s in the game folder. "
              "Please send that file with your bug report.",
              name);
  } else {
    wsprintfA(msg, "The game crashed, and the crash report couldn't be saved (error %lu).", GetLastError());
  }
  MessageBoxA(NULL, msg, "Animal Crossing - Crash", MB_OK | MB_ICONERROR | MB_TOPMOST);
  return EXCEPTION_EXECUTE_HANDLER;
}

void pc_crash_init(void) {
  s_start_ms = GetTickCount();
  SetUnhandledExceptionFilter(pc_crash_handler);
}

void pc_crash_init_info(void) {
  typedef const char*(CDECL * WineVersionFn)(void);
  typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW*);
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  WineVersionFn wine = ntdll ? (WineVersionFn)(void*)GetProcAddress(ntdll, "wine_get_version") : NULL;
  RtlGetVersionFn rtl = ntdll ? (RtlGetVersionFn)(void*)GetProcAddress(ntdll, "RtlGetVersion") : NULL;
  OSVERSIONINFOW v;
  const char* vendor = (const char*)glGetString(GL_VENDOR);
  const char* renderer = (const char*)glGetString(GL_RENDERER);
  const char* version = (const char*)glGetString(GL_VERSION);

  ZeroMemory(&v, sizeof(v));
  v.dwOSVersionInfoSize = sizeof(v);
  // RtlGetVersion: GetVersionEx lies to unmanifested exes
  if(rtl != NULL && rtl(&v) == 0) {
    snprintf(s_os, sizeof(s_os), "Windows %lu.%lu build %lu", v.dwMajorVersion, v.dwMinorVersion,
             v.dwBuildNumber);
  }
  if(wine != NULL) {
    size_t len = strlen(s_os);
    snprintf(s_os + len, sizeof(s_os) - len, " (Wine %s)", wine());
  }
  snprintf(s_gl, sizeof(s_gl), "%s | %s | %s", vendor ? vendor : "?", renderer ? renderer : "?",
           version ? version : "?");
  s_module_count_init = crash_collect_modules(s_modules_init);
  // Again, in case SDL or the GL driver installed their own
  SetUnhandledExceptionFilter(pc_crash_handler);
}

void pc_crash_note_frame(GAME_PLAY* play) {
  CrashFrame f;
  PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);
  int scene = Save_Get(scene_no);

  f.frame = s_frame.frame + 1;
  f.tick_ms = GetTickCount();
  f.scene = scene;
  f.date = Common_Get(time.rtc_time);
  f.weather = mEnv_SAVE_GET_WEATHER_TYPE(Save_Get(weather));
  f.weather_intensity = mEnv_SAVE_GET_WEATHER_INTENSITY(Save_Get(weather));
  f.has_player = player != NULL;
  if(player != NULL) {
    f.player_x = (int)player->actor_class.world.position.x;
    f.player_y = (int)player->actor_class.world.position.y;
    f.player_z = (int)player->actor_class.world.position.z;
  } else {
    f.player_x = f.player_y = f.player_z = 0;
  }
  f.ap_state = ap_getconnectstate()->state;
  s_frame = f;

  if(scene != s_last_scene) {
    CrashScene* s = &s_scenes[s_scene_count % CRASH_SCENES];
    s->frame = f.frame;
    s->tick_ms = f.tick_ms;
    s->from = s_last_scene;
    s->to = scene;
    zelda_GetFreeArena(&s->heap_max_block, &s->heap_free, &s->heap_alloc);
    s_scene_count++;
    s_last_scene = scene;
  }
}

void pc_crash_test(void) {
  volatile int* p = NULL;
  *p = 1;
}

#else

void pc_crash_init(void) {}
void pc_crash_init_info(void) {}
void pc_crash_note_frame(struct game_play_s* play) { (void)play; }
void pc_crash_test(void) { abort(); }

#endif
