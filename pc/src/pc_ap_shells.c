#include "pc_ap_shells.h"

#include "m_name_table.h"

mActor_name_t pc_ap_shells_pick(mActor_name_t shell, int is_island, int coconuts, int roll) {
  // Coconuts Wash Up: town only, the island has its own coconuts
  if (coconuts && !is_island && roll < PC_AP_SHELLS_COCONUT_PERCENT) {
    return ITM_FOOD_COCONUT;
  }
  return shell;
}
