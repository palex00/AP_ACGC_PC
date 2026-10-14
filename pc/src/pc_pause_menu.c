#include "pc_platform.h"
#include "pc_pause_menu.h"
#include "pc_settings_menu.h"
#include "pc_menu_util.h"
#include "pc_text_draw.h"
#include "pc_ap_time_menu.h"
#include "pc_ap_catalog_menu.h"
#include "pc_ap_critter_menu.h"
#include "pc_ap_shell_menu.h"
#include "pc_ap_logic.h"
#include "pc_ap_overlay.h"

#include "m_font.h"
#include "m_museum_display.h" /* mMmd_CATEGORY_* */
#include "m_rcp.h"
#include "graph.h"
#include "main.h"       /* SCREEN_WIDTH_F */

#include <stdio.h>
#include <string.h>

int g_pc_paused = 0;

/* Pausing is blocked while either is set: main menu (set by ac_animal_logo)
 * or NES running (its own pause, and pausing it crashes the game). */
int g_pc_title_main_menu_visible = 0;
int g_pc_nes_active = 0;

/* Drains the keys still held from confirming Resume so they don't leak into play. */
int g_pc_pause_input_drain = 0;

/* Pause menu pages */
typedef enum {
    PAGE_MAIN = 0,
    PAGE_SETTINGS = 1,
    PAGE_CONFIRM_QUIT = 2,
    PAGE_TIME = 3,
    PAGE_CATALOG = 4,
    PAGE_CRITTERS = 5, /* Bugs or Fish */
    PAGE_SHELLS = 6,
} PauseMenuPage;

// Main page entries; which ones show is decided by main_update
typedef enum {
    MAIN_RESUME,
    MAIN_TIME,
    MAIN_SAVE,
    MAIN_CATALOG, // Catalogsanity only
    MAIN_BUGS,
    MAIN_FISH,
    MAIN_SHELLS, // Shellsanity only
    MAIN_SETTINGS,
    MAIN_QUIT,
    MAIN_MAX,
} MainItem;

#define CONFIRM_ITEM_COUNT  2

static PauseMenuPage cur_page = PAGE_MAIN;
static MainItem main_items[MAIN_MAX];
static int main_count = 0;
static int main_sel = 0;    /* index into main_items */
static int confirm_sel = 0; /* 0=No (default), 1=Yes */

// Page state
static pc_ap_time_menu time_menu;
static pc_ap_catalog_menu catalog_menu;
static pc_ap_critter_menu critter_menu; // Bugs or Fish
static pc_ap_shell_menu shell_menu;

// Rebuilds the main page entries (Catalogsanity can turn on while paused on the title)
static void main_update(void) {
    main_count = 0;
    main_items[main_count++] = MAIN_RESUME;
    main_items[main_count++] = MAIN_TIME;
    main_items[main_count++] = MAIN_SAVE;
    if (pc_ap_catalog_pools_total() > 0) {
        main_items[main_count++] = MAIN_CATALOG;
    }
    main_items[main_count++] = MAIN_BUGS;
    main_items[main_count++] = MAIN_FISH;
    if (pc_ap_shells_per_month() > 0) {
        main_items[main_count++] = MAIN_SHELLS;
    }
    main_items[main_count++] = MAIN_SETTINGS;
    main_items[main_count++] = MAIN_QUIT;
    if (main_sel >= main_count) main_sel = 0;
}

static const char* main_label(MainItem item) {
    switch (item) {
        case MAIN_RESUME:   return "Resume";
        case MAIN_TIME:     return "Date & Time";
        case MAIN_SAVE:     return "Save";
        case MAIN_CATALOG:  return "Catalog";
        case MAIN_BUGS:     return "Bugs";
        case MAIN_FISH:     return "Fish";
        case MAIN_SHELLS:   return "Shells";
        case MAIN_SETTINGS: return "Settings";
        case MAIN_QUIT:     return "Quit Game";
        default:            return "";
    }
}

// Greyed out where it can't open
static int main_disabled(MainItem item) {
    if (item == MAIN_TIME || item == MAIN_SAVE) return !pc_ap_time_change_allowed_now();
    if (item == MAIN_CATALOG || item == MAIN_BUGS || item == MAIN_FISH || item == MAIN_SHELLS) return !pc_ap_accepting();
    return 0;
}

int pc_pause_menu_submenu_open(void) {
    return g_pc_paused && cur_page != PAGE_MAIN;
}

void pc_pause_menu_toggle(void) {
    if (!g_pc_paused && (g_pc_title_main_menu_visible || g_pc_nes_active)) return;

    g_pc_paused = !g_pc_paused;
    if (g_pc_paused) {
        cur_page = PAGE_MAIN;
        main_sel = 0;
    } else {
        g_pc_pause_input_drain = 1;
    }
}

/* Input */

static void main_activate(void) {
    switch (main_items[main_sel]) {
        case MAIN_RESUME:
            pc_pause_menu_toggle();
            break;
        case MAIN_TIME: /* only where the gate allows it */
            if (pc_ap_time_change_allowed_now()) {
                cur_page = PAGE_TIME;
                pc_ap_time_menu_enter(&time_menu);
            } else {
                pc_ap_overlay_toast("Date & Time: only outdoors in your own town");
            }
            break;
        case MAIN_SAVE: /* same gate as Date & Time; the reload starts on the next play frame */
            if (pc_ap_time_change_allowed_now()) {
                pc_ap_save_request();
                pc_pause_menu_toggle();
            } else {
                pc_ap_overlay_toast("Save: only outdoors in your own town");
            }
            break;
        case MAIN_CATALOG: /* needs a loaded save (the catalog is the player's) */
            if (pc_ap_accepting()) {
                cur_page = PAGE_CATALOG;
                pc_ap_catalog_menu_enter(&catalog_menu);
            } else {
                pc_ap_overlay_toast("Catalog: load your save first");
            }
            break;
        case MAIN_BUGS: /* the journal is the player's */
        case MAIN_FISH:
            if (pc_ap_accepting()) {
                cur_page = PAGE_CRITTERS;
                pc_ap_critter_menu_enter(&critter_menu, main_items[main_sel] == MAIN_FISH ? mMmd_CATEGORY_FISH : mMmd_CATEGORY_INSECT);
            } else {
                pc_ap_overlay_toast(main_items[main_sel] == MAIN_FISH ? "Fish: load your save first"
                                                                      : "Bugs: load your save first");
            }
            break;
        case MAIN_SHELLS:
            if (pc_ap_accepting()) {
                cur_page = PAGE_SHELLS;
                pc_ap_shell_menu_enter(&shell_menu);
            } else {
                pc_ap_overlay_toast("Shells: load your save first");
            }
            break;
        case MAIN_SETTINGS:
            cur_page = PAGE_SETTINGS;
            pc_settings_menu_enter();
            break;
        case MAIN_QUIT: /* -> confirm page (default to No) */
            cur_page = PAGE_CONFIRM_QUIT;
            confirm_sel = 0;
            break;
        default:
            break;
    }
}

static void confirm_activate(void) {
    if (confirm_sel == 1) {
        printf("[PAUSE] Quit confirmed\n");
        g_pc_running = 0;
    } else {
        cur_page = PAGE_MAIN;
        main_sel = 0;
    }
}

/* Keyboard and gamepad both map to pc_menu_action (pc_menu_util.h). */
static void handle_action(pc_menu_action act) {
    /* Settings page is driven by pc_settings_menu. */
    if (cur_page == PAGE_SETTINGS) {
        switch (act) {
            case PC_MENU_UP:    pc_settings_menu_nav_up();    break;
            case PC_MENU_DOWN:  pc_settings_menu_nav_down();  break;
            case PC_MENU_LEFT:  pc_settings_menu_nav_left();  break;
            case PC_MENU_RIGHT: pc_settings_menu_nav_right(); break;
            case PC_MENU_CONFIRM:
                if (!pc_settings_menu_confirm()) cur_page = PAGE_MAIN;
                break;
            case PC_MENU_CANCEL:
                if (!pc_settings_menu_cancel()) cur_page = PAGE_MAIN;
                break;
            default: break;
        }
        return;
    }

    /* Date & Time, Catalog, Bugs/Fish, Shells: each page handles its own input. */
    if (cur_page == PAGE_TIME || cur_page == PAGE_CATALOG || cur_page == PAGE_CRITTERS || cur_page == PAGE_SHELLS) {
        int result;
        if (cur_page == PAGE_TIME)         result = pc_ap_time_menu_input(&time_menu, act);
        else if (cur_page == PAGE_CATALOG) result = pc_ap_catalog_menu_input(&catalog_menu, act);
        else if (cur_page == PAGE_SHELLS)  result = pc_ap_shell_menu_input(&shell_menu, act);
        else                               result = pc_ap_critter_menu_input(&critter_menu, act);
        if (result == PC_MENU_BACK) {
            cur_page = PAGE_MAIN;
        } else if (result == PC_MENU_RESUME) {
            pc_pause_menu_toggle();
        }
        return;
    }

    /* Pages owned here (Main, Quit-confirm). */
    main_update();
    {
        int item_count = (cur_page == PAGE_MAIN) ? main_count : CONFIRM_ITEM_COUNT;
        int* sel       = (cur_page == PAGE_MAIN) ? &main_sel : &confirm_sel;

        switch (act) {
            case PC_MENU_UP:
            case PC_MENU_LEFT:
                *sel = (*sel + item_count - 1) % item_count;
                break;
            case PC_MENU_DOWN:
            case PC_MENU_RIGHT:
                *sel = (*sel + 1) % item_count;
                break;
            case PC_MENU_CONFIRM:
                if (cur_page == PAGE_MAIN)              main_activate();
                else if (cur_page == PAGE_CONFIRM_QUIT) confirm_activate();
                break;
            case PC_MENU_CANCEL:
                if (cur_page == PAGE_CONFIRM_QUIT) cur_page = PAGE_MAIN;
                else                               pc_pause_menu_toggle();
                break;
            default: break;
        }
    }
}

static pc_menu_action translate_key(SDL_Keycode k) {
    switch (k) {
        case SDLK_UP:
        case SDLK_w:        return PC_MENU_UP;
        case SDLK_DOWN:
        case SDLK_s:        return PC_MENU_DOWN;
        case SDLK_LEFT:
        case SDLK_a:        return PC_MENU_LEFT;
        case SDLK_RIGHT:
        case SDLK_d:        return PC_MENU_RIGHT;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:    return PC_MENU_CONFIRM;
        case SDLK_ESCAPE:   return PC_MENU_CANCEL;
    }
    return PC_MENU_NONE;
}

static pc_menu_action translate_pad_button(int button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    return PC_MENU_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  return PC_MENU_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  return PC_MENU_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return PC_MENU_RIGHT;
        case SDL_CONTROLLER_BUTTON_A:          return PC_MENU_CONFIRM;
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_BACK:
        case SDL_CONTROLLER_BUTTON_START:      return PC_MENU_CANCEL;
    }
    return PC_MENU_NONE;
}

/* Left-stick nav with a per-axis latch so one push = one step. */
#define STICK_NAV_PRESS   18000
#define STICK_NAV_RELEASE 12000

static pc_menu_action translate_pad_axis(int axis, int value) {
    static int latch_x = 0, latch_y = 0;
    int* latch;
    pc_menu_action neg, pos;

    if (axis == SDL_CONTROLLER_AXIS_LEFTX)      { latch = &latch_x; neg = PC_MENU_LEFT; pos = PC_MENU_RIGHT; }
    else if (axis == SDL_CONTROLLER_AXIS_LEFTY) { latch = &latch_y; neg = PC_MENU_UP;   pos = PC_MENU_DOWN; }
    else return PC_MENU_NONE;

    if (*latch != 0) {
        if (value > -STICK_NAV_RELEASE && value < STICK_NAV_RELEASE) *latch = 0;
        return PC_MENU_NONE;
    }
    if (value <= -STICK_NAV_PRESS) { *latch = -1; return neg; }
    if (value >=  STICK_NAV_PRESS) { *latch =  1; return pos; }
    return PC_MENU_NONE;
}

int pc_pause_menu_handle_event(const SDL_Event* e) {
    if (!g_pc_paused) return 0;

    switch (e->type) {
        case SDL_KEYDOWN:
            if (e->key.repeat) return 1;
            handle_action(translate_key(e->key.keysym.sym));
            return 1; /* swallow all keys while paused */
        case SDL_CONTROLLERBUTTONDOWN:
            handle_action(translate_pad_button(e->cbutton.button));
            return 1;
        case SDL_CONTROLLERAXISMOTION:
            handle_action(translate_pad_axis(e->caxis.axis, e->caxis.value));
            return 1;
    }
    return 0;
}

/* Drawing */

static void draw_main_page(struct game_s* game) {
    main_update();
    // Center title + entries vertically (y is a row's top, rows are 16 high), then up a bit:
    // the AP status and seed lines take the bottom ~25 px
    f32 line_h = 17.0f;
    f32 title_gap = 30.0f; // title top to first entry top
    f32 shift_up = 10.0f;
    f32 block_h = title_gap + (main_count - 1) * line_h + mFont_TEX_CHAR_HEIGHT_F32;
    f32 top = (SCREEN_HEIGHT_F - block_h) * 0.5f - shift_up;
    pc_menu_draw_centered(game, "- Paused -", top, 255, 255, 255, 255, 1.0f);

    f32 y = top + title_gap;
    for (int i = 0; i < main_count; i++) {
        int r, g, b, a;
        int selected = (i == main_sel);
        pc_menu_row_colors(selected, &r, &g, &b, &a);
        if (main_disabled(main_items[i])) {
            r = g = b = 120;
            a = selected ? 220 : 160;
        }
        pc_menu_draw_centered(game, main_label(main_items[i]), y + i * line_h, r, g, b, a,
                              selected ? PC_MENU_SCALE_SELECTED : 1.0f);
    }
}

static void draw_confirm_page(struct game_s* game) {
    pc_menu_draw_centered(game, "- Quit Game -", 80.0f, 255, 255, 255, 255, 1.0f);
    pc_menu_draw_centered(game, "Are you sure you want to quit?",
                          115.0f, 230, 230, 230, 255, 1.0f);
    pc_menu_draw_two_choice(game, "No", "Yes", confirm_sel, 150.0f);
}

void pc_pause_menu_draw(struct game_s* game) {
    if (!g_pc_paused || game == NULL || game->graph == NULL) return;

    /* Font ortho projection + identity modelview, else our screen-space quads
     * inherit whatever projection the scene ended its frame with. */
    mFont_SetMatrix(game->graph, mFont_MODE_FONT);
    pc_settings_menu_tick();

    if (cur_page == PAGE_SETTINGS) {
        /* Settings module owns its own dim backdrop. */
        pc_settings_menu_draw(game, /*with_dim_backdrop=*/1);
        /* The user may have closed it from inside (Back). */
        if (!pc_settings_menu_active()) cur_page = PAGE_MAIN;
    } else if (cur_page == PAGE_TIME) {
        pc_ap_time_menu_draw(&time_menu, game);
    } else if (cur_page == PAGE_CATALOG) {
        pc_ap_catalog_menu_draw(&catalog_menu, game);
    } else if (cur_page == PAGE_CRITTERS) {
        pc_ap_critter_menu_draw(&critter_menu, game);
    } else if (cur_page == PAGE_SHELLS) {
        pc_ap_shell_menu_draw(&shell_menu, game);
    } else {
        pc_menu_dim_rect(game->graph, 180);
        if (cur_page == PAGE_MAIN)              draw_main_page(game);
        else if (cur_page == PAGE_CONFIRM_QUIT) draw_confirm_page(game);
    }

    mFont_UnSetMatrix(game->graph, mFont_MODE_FONT);
}
