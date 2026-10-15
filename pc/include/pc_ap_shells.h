// Coconuts Wash Up (QoL): some town shells are coconuts. Called from the facade's
// pc_ap_shell_item (pc_ap_logic.h), where every washed-up shell goes through.
#ifndef PC_AP_SHELLS_H
#define PC_AP_SHELLS_H

#include "types.h"
#include "m_actor_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// Chance (percent) that a town shell washes up as a coconut with Coconuts Wash Up on.
// ~20 shells per full beach walk (research/coconuts.md): about one coconut every two to three walks.
#define PC_AP_SHELLS_COCONUT_PERCENT 2

// The item to wash up instead of the game's shell: a coconut in town when coconuts is on and
// the roll (0-99) hits, else shell
mActor_name_t pc_ap_shells_pick(mActor_name_t shell, int is_island, int coconuts, int roll);

#ifdef __cplusplus
}
#endif

#endif
