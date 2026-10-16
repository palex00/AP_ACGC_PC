// Extra Bells: the game's 10,000 and 30,000 Bell bags as AP items (AP item id ==
// the game's item number), each mailed once with the sender's name.
// Reads AP state (received items), never writes to the DLL or the game:
// the pc_ap_logic.c facade does the mailing.
#ifndef PC_AP_MONEY_H
#define PC_AP_MONEY_H

#include "types.h"
#include "m_actor_type.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ini_t;

typedef struct {
  // Persisted
  int mailed; // Bell bags mailed so far, in received order
} pc_ap_money;

void pc_ap_money_init(pc_ap_money* m);
void pc_ap_money_load(pc_ap_money* m, struct ini_t* ini);
void pc_ap_money_save(const pc_ap_money* m, struct ini_t* ini);

// Next received Bell bag that isn't mailed yet: its index in the received item
// list (for the item and sender), -1 when done
int pc_ap_money_next_unmailed(const pc_ap_money* m);
void pc_ap_money_mark_mailed(pc_ap_money* m);
// Letter body. sender = AP player name, NULL for none (server, start inventory);
// own = found in our own world.
void pc_ap_money_letter(char* buf, size_t len, const char* sender, int own);

#ifdef __cplusplus
}
#endif

#endif
