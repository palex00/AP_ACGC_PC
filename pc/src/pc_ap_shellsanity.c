#include "pc_ap_shellsanity.h"
#include "ap_archipelago.h"

#include <stdio.h>

void pc_ap_shellsanity_init(pc_ap_shellsanity* s, const ap_slotdata* sd) {
  s->per_month = sd->valid ? sd->beach_per_month : 0;
  s->chance = sd->beach_item_chance;
}

int pc_ap_shellsanity_is_item(mActor_name_t item) {
  return item >= PC_AP_BEACH_ITEM_START && item < PC_AP_BEACH_ITEM_END;
}

int64_t pc_ap_shellsanity_location(int month, int k) {
  return PC_AP_LOC_BEACH_BASE + month * 0x100 + k;
}

int pc_ap_shellsanity_left(const pc_ap_shellsanity* s, int month) {
  int64_t ids[AP_BEACH_PER_MONTH_MAX];
  uint8_t checked[AP_BEACH_PER_MONTH_MAX];
  int left = 0;

  if (s->per_month <= 0 || month < 0 || month >= PC_AP_MONTHS) {
    return 0;
  }
  for (int k = 1; k <= s->per_month; k++) {
    ids[k - 1] = pc_ap_shellsanity_location(month, k);
  }
  ap_locations_checked(ids, s->per_month, checked);
  for (int i = 0; i < s->per_month; i++) {
    left += !checked[i];
  }
  return left;
}

mActor_name_t pc_ap_shellsanity_wash_up(const pc_ap_shellsanity* s, mActor_name_t shell, int month, int on_beach,
                                        int roll) {
  if (s->per_month <= 0 || roll >= s->chance) {
    return shell;
  }
  if (on_beach >= pc_ap_shellsanity_left(s, month)) {
    return shell;
  }
  return PC_AP_BEACH_ITEM_START + month;
}

int64_t pc_ap_shellsanity_pickup(const pc_ap_shellsanity* s, mActor_name_t item) {
  int month = item - PC_AP_BEACH_ITEM_START;

  if (!pc_ap_shellsanity_is_item(item)) {
    return -1;
  }
  for (int k = 1; k <= s->per_month; k++) {
    int64_t id = pc_ap_shellsanity_location(month, k);
    if (!ap_location_checked(id)) {
      return id;
    }
  }
  return -1;
}

void pc_ap_shellsanity_name(mActor_name_t item, char* out, int len) {
  static const char* months[PC_AP_MONTHS] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  int month = item - PC_AP_BEACH_ITEM_START;
  int n;

  if (!pc_ap_shellsanity_is_item(item)) {
    month = 0;
  }
  n = snprintf(out, len + 1, "AP Shell %s", months[month]);
  for (; n < len; n++) {
    out[n] = ' ';
  }
  out[len] = 0;
}
