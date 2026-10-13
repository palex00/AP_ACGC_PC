#include "pc_ap_store.h"
#include "ap_archipelago.h"
#include "m_name_table.h"

#include <string.h>

// Journal bits in furniture_collected_bitfield, by furniture index (m_submenu.c
// mSM_COLLECT_INSECT_SET / mSM_COLLECT_FISH_SET)
#define PC_AP_STORE_INSECT_IDX 0x2F2
#define PC_AP_STORE_FISH_IDX   0x31A

void pc_ap_store_init(pc_ap_store* s, const ap_slotdata* sd) {
  memset(s, 0, sizeof(*s));
  s->mode = sd->store_upgrades;
  for(int i = 0; i < AP_STORE_LEVELS; i++) {
    s->thresholds[i] = sd->store_thresholds[i] > 0 ? (u32)sd->store_thresholds[i] : 0;
  }
}

int pc_ap_store_level(const pc_ap_store* s, int cur_level, u32 sales_sum, int catalog_count) {
  int level = 0;
  if(s->mode == AP_STORE_ITEMS) {
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

static int pc_ap_store_popcount(u32 v) {
  int n = 0;
  while(v != 0) {
    v &= v - 1;
    n++;
  }
  return n;
}

static int pc_ap_store_count_bits(const u32* bits, int words) {
  int n = 0;
  for(int i = 0; i < words; i++) {
    n += pc_ap_store_popcount(bits[i]);
  }
  return n;
}

static int pc_ap_store_journal_bit(int idx) {
  return (idx >= PC_AP_STORE_INSECT_IDX && idx < PC_AP_STORE_INSECT_IDX + INSECT_ONLY_NUM) ||
         (idx >= PC_AP_STORE_FISH_IDX && idx < PC_AP_STORE_FISH_IDX + FISH_NUM);
}

int pc_ap_store_catalog_count(const Private_c* priv) {
  if(priv == NULL) {
    return 0;
  }
  int words = sizeof(priv->furniture_collected_bitfield) / sizeof(u32);
  int n = 0;
  for(int idx = 0; idx < words * 32; idx++) {
    if((priv->furniture_collected_bitfield[idx >> 5] & (1u << (idx & 31))) && !pc_ap_store_journal_bit(idx)) {
      n++;
    }
  }
  n += pc_ap_store_count_bits(priv->wall_collected_bitfield, 3);
  n += pc_ap_store_count_bits(priv->carpet_collected_bitfield, 3);
  n += pc_ap_store_count_bits(priv->paper_collected_bitfield, 2);
  n += pc_ap_store_count_bits(priv->music_collected_bitfield, 2);
  return n;
}
