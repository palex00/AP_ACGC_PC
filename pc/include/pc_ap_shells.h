// Beach shells. Every shell the game washes up (mFI_GetShell, town and island) goes through
// pc_ap_shell_item, the one place for shell options.
#ifndef PC_AP_SHELLS_H
#define PC_AP_SHELLS_H

#include "types.h"
#include "m_actor_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// Chance (percent) that a town shell washes up as a coconut with Coconuts Wash Up on.
// ~20 shells per full beach walk (research/coconuts.md): about one coconut every two walks.
#define PC_AP_SHELLS_COCONUT_PERCENT 3

// The item to wash up instead of the game's pick (shell, or a coconut on the island)
mActor_name_t pc_ap_shell_item(mActor_name_t shell, int is_island);

// pc_ap_shell_item without slot_data or RNG: roll is 0-99
mActor_name_t pc_ap_shells_pick(mActor_name_t shell, int is_island, int coconuts, int roll);

#ifdef __cplusplus
}
#endif

#endif
