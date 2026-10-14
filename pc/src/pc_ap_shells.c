#include "pc_ap_shells.h"
#include "ap_slotdata.h"

#include "m_name_table.h"
#include "libc64/qrand.h"

mActor_name_t pc_ap_shells_pick(mActor_name_t shell, int is_island, int coconuts, int roll) {
  // Coconuts Wash Up: town only, the island has its own coconuts
  if (coconuts && !is_island && roll < PC_AP_SHELLS_COCONUT_PERCENT) {
    return ITM_FOOD_COCONUT;
  }
  return shell;
}

mActor_name_t pc_ap_shell_item(mActor_name_t shell, int is_island) {
  ap_slotdata* sd = ap_getslotdata();
  int coconuts = sd->valid && sd->coconuts_wash_up;
  return pc_ap_shells_pick(shell, is_island, coconuts, RANDOM(100));
}
