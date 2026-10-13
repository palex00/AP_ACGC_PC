#include "pc_ap_store.h"
#include "ap_archipelago.h"

#include <string.h>

void pc_ap_store_init(pc_ap_store* s, const ap_slotdata* sd) {
  memset(s, 0, sizeof(*s));
  s->mode = sd->store_upgrades;
  for(int i = 0; i < AP_STORE_LEVELS; i++) {
    s->thresholds[i] = sd->store_thresholds[i] > 0 ? (u32)sd->store_thresholds[i] : 0;
  }
}

int pc_ap_store_level(const pc_ap_store* s, int cur_level, u32 sales_sum, int catalog_count) {
  int level = 0;
  if(s->mode == AP_STORE_PROGRESSIVE) {
    level = ap_item_count(PC_AP_ITEM_PROGRESSIVE_NOOK_STORE);
  } else if(s->mode == AP_STORE_SALES || s->mode == AP_STORE_CATALOG) {
    u32 value = s->mode == AP_STORE_SALES ? sales_sum : (u32)catalog_count;
    while(level < AP_STORE_LEVELS && value >= s->thresholds[level]) {
      level++;
    }
  } else {
    return -1;
  }

  if(level > AP_STORE_LEVELS) {
    level = AP_STORE_LEVELS;
  }
  if(level > cur_level + 1) {
    level = cur_level + 1;
  }
  if(level < cur_level) {
    level = cur_level;
  }
  return level;
}

int pc_ap_store_sales_cap(const pc_ap_store* s, int cur_level, u32* cap) {
  if(s->mode != AP_STORE_SALES) {
    return 0;
  }
  // Nookington's: nothing left to reach
  *cap = cur_level < AP_STORE_LEVELS ? s->thresholds[cur_level] : 0xFFFFFFFF;
  return 1;
}
