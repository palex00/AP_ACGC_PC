#include "pc_ap_catalogsanity.h"
#include "ap_archipelago.h"
#include "m_shop.h"

#include <string.h>

void pc_ap_catalogsanity_init(pc_ap_catalogsanity* c, const ap_slotdata* sd) {
  memset(c, 0, sizeof(*c));
  if(!sd->catalogsanity) {
    return;
  }
  c->count = sd->catalog_count;
  for(int i = 0; i < c->count; i++) {
    c->items[i] = sd->catalog_items[i];
    c->pool[i] = sd->catalog_pool[i];
  }
}

int pc_ap_catalogsanity_open_pools(const pc_ap_catalogsanity* c) {
  if(c->count == 0) {
    return 0;
  }
  return 1 + ap_item_count(PC_AP_ITEM_PROGRESSIVE_CATALOG_POOL);
}

int64_t pc_ap_catalogsanity_next_check(const pc_ap_catalogsanity* c, int open, int* it) {
  while(*it < c->count) {
    int i = (*it)++;
    if(c->pool[i] < open && mSP_CollectCheck(c->items[i])) {
      return PC_AP_LOC_CATALOG_BASE + c->items[i];
    }
  }
  return -1;
}
