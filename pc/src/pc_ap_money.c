#include "pc_ap_money.h"
#include "pc_ap_state.h"
#include "ap_archipelago.h"
#include "m_name_table.h"

#include <stdio.h>
#include <string.h>

#define PC_AP_MONEY_SECTION "money"
#define PC_AP_MONEY_KEY_MAILED "mailed"

static int pc_ap_money_is_bag(int64_t id) {
  return id == ITM_MONEY_10000 || id == ITM_MONEY_30000;
}

void pc_ap_money_init(pc_ap_money* m) {
  memset(m, 0, sizeof(*m));
}

void pc_ap_money_load(pc_ap_money* m, struct ini_t* ini) {
  m->mailed = pc_ap_ini_get_int(ini, PC_AP_MONEY_SECTION, PC_AP_MONEY_KEY_MAILED, 0);
}

void pc_ap_money_save(const pc_ap_money* m, struct ini_t* ini) {
  pc_ap_ini_set_int(ini, PC_AP_MONEY_SECTION, PC_AP_MONEY_KEY_MAILED, m->mailed);
}

int pc_ap_money_next_unmailed(const pc_ap_money* m) {
  size_t n = ap_getitemcount();
  int seen = 0;
  for(size_t k = 0; k < n; k++) {
    if(!pc_ap_money_is_bag(ap_getitem(k))) {
      continue;
    }
    if(seen == m->mailed) {
      return (int)k;
    }
    seen++;
  }
  return -1;
}

void pc_ap_money_mark_mailed(pc_ap_money* m) {
  m->mailed++;
}

void pc_ap_money_letter(char* buf, size_t len, const char* sender, int own) {
  if(own) {
    snprintf(buf, len, "You found\nsome Bells!");
  } else if(sender != NULL) {
    snprintf(buf, len, "%s\nsent you some Bells!", sender);
  } else {
    snprintf(buf, len, "Here are\nsome Bells!");
  }
}
