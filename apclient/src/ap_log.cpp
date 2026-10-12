#include "ap_log.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <thread>

#define AP_LOG_RING_SIZE 512

static std::mutex g_mtx;
static ApLogEntry g_ring[AP_LOG_RING_SIZE];
static uint64_t g_next_seq = 0; // total entries ever written

void ap_log_write(const char * level, const char* fmt, ...) {
  ApLogEntry e;
  e.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch())
    .count();
  va_list args;
  va_start(args, fmt);
  vsnprintf(e.msg, sizeof e.msg, fmt, args); // truncates, always terminates
  va_end(args);

  std::lock_guard<std::mutex> lock(g_mtx);
  e.seq = g_next_seq++;
  g_ring[e.seq % AP_LOG_RING_SIZE] = e;
  fprintf(stderr, "[AP][%s] %s\n", level, e.msg);
}

// Caller holds g_mtx
static size_t ap_log_copy(size_t max_count, ApLogEntry* out) {
  if(max_count > AP_LOG_RING_SIZE) {
    max_count = AP_LOG_RING_SIZE;
  }
  size_t s = 0;
  for (s = 0; s < max_count; s++) {
    if(s >= g_next_seq) { break; }
    out[s] = g_ring[(g_next_seq - s - 1) % AP_LOG_RING_SIZE];
  }
  return s;
}

size_t ap_log_get(size_t max_count, ApLogEntry* out) {
  if (!out || max_count == 0) { return 0; }
  std::lock_guard<std::mutex> lock(g_mtx);
  return ap_log_copy(max_count, out);
}

size_t ap_log_try_get(size_t max_count, ApLogEntry* out) {
  if (!out || max_count == 0) { return 0; }
  // ~50ms total: long enough for another thread to finish a write, short enough not to
  // hang a crash report if the crashed thread itself holds the lock
  for (int tries = 0; tries < 50; tries++) {
    if (g_mtx.try_lock()) {
      size_t n = ap_log_copy(max_count, out);
      g_mtx.unlock();
      return n;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return 0;
}
