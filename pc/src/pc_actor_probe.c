// pc_actor_probe.c - --debug: logs actors that jump far in one frame (see pc_actor_probe.h)
#include "pc_actor_probe.h"
#include "pc_platform.h"
#include "m_actor.h"
#include "m_play.h"
#include "m_common_data.h"

#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PROBE_JUMP 40.0f     // units per frame; the player runs ~4-8
#define PROBE_FIX_TOL 0.01f  // fixed point step is 1/65536
#define PROBE_ACTORS 1024    // power of two
#define PROBE_MTXS 8192      // power of two
#define PROBE_SEARCH 32
#define PROBE_LINES_PER_ACTOR 20
#define PROBE_LINES_MAX 5000
#define PROBE_JOINTS 64      // draw matrices remembered per actor (skeleton joints, held items, ...)

int g_pc_actor_probe = 0;

typedef struct {
    ACTOR* actor;
    u32 pos_frame;  // frame of pos (0 = empty)
    xyz_t pos;
    u32 draw_frame; // frame of draw_pos
    xyz_t draw_pos; // translation of the actor's first draw matrix that frame
    s16 id;
    int lines;
    u32 joint_frame;            // frame of joints[]
    int joint_n;
    xyz_t joints[PROBE_JOINTS]; // translation of each draw matrix, in draw order
    u32 cur_frame;              // frame of cur[] (being filled)
    int cur_n;
    xyz_t cur[PROBE_JOINTS];
} ProbeActor;

typedef struct {
    const void* mtx;
    u32 frame;
    ProbeActor* owner;
    s32 words[16];
} ProbeMtx;

static FILE* s_log;
static int s_lines;
static u32 s_frame;          // current game frame (play->game_frame + 1, so 0 means none)
static GAME_PLAY* s_play;
static ProbeActor* s_cur;    // actor being drawn
static int s_cur_has_mtx;    // its first matrix this frame is recorded
static xyz_t s_cam_eye, s_cam_center;
static u32 s_cam_frame;
static ProbeActor s_actors[PROBE_ACTORS];
static ProbeMtx s_mtxs[PROBE_MTXS];

static f32 dist(const xyz_t* a, const xyz_t* b) {
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;
    f32 dz = a->z - b->z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// Prints one log line with the common prefix; returns 0 when the caps are hit.
static int log_line(ProbeActor* a, const char* kind) {
    if (s_log == NULL || s_lines >= PROBE_LINES_MAX) {
        return 0;
    }
    if (a != NULL) {
        if (a->lines >= PROBE_LINES_PER_ACTOR) {
            return 0;
        }
        a->lines++;
    }
    s_lines++;
    fprintf(s_log, "f=%u t=%ums scene=%d %-6s", (unsigned)(s_frame - 1), (unsigned)SDL_GetTicks(), Save_Get(scene_no), kind);
    if (a != NULL) {
        fprintf(s_log, " actor=%p id=0x%04x npc=0x%04x", (void*)a->actor, (u16)a->actor->id,
                (u16)a->actor->npc_id);
    }
    return 1;
}

static void log_end(void) {
    fputc('\n', s_log);
    fflush(s_log);
}

void pc_actor_probe_init(void) {
    if (!g_pc_actor_probe) {
        return;
    }
    s_log = fopen("actor_probe.txt", "w");
    if (s_log == NULL) {
        g_pc_actor_probe = 0;
        return;
    }
    fprintf(s_log, "actor probe: jump > %.0f units/frame, fixed-point tolerance %.3f\n", PROBE_JUMP, PROBE_FIX_TOL);
    fflush(s_log);
}

static ProbeActor* find_actor(ACTOR* actor) {
    u32 h = ((uintptr_t)actor >> 4) * 2654435761u;
    ProbeActor* free_slot = NULL;
    int i;

    for (i = 0; i < PROBE_SEARCH; i++) {
        ProbeActor* a = &s_actors[(h + i) & (PROBE_ACTORS - 1)];
        // same pointer, same actor type: same actor (a freed and reused slot is a new one)
        if (a->actor == actor && a->id == actor->id && a->pos_frame + 60 >= s_frame) {
            return a;
        }
        // not drawn for a second: free
        if (free_slot == NULL && (a->pos_frame == 0 || a->pos_frame + 60 < s_frame)) {
            free_slot = a;
        }
    }
    if (free_slot != NULL) {
        memset(free_slot, 0, sizeof(*free_slot));
        free_slot->actor = actor;
        free_slot->id = actor->id;
    }
    return free_slot;
}

static void check_camera(GAME_PLAY* play) {
    xyz_t eye = play->camera.lookat.eye;
    xyz_t center = play->camera.lookat.center;

    if (s_cam_frame + 1 == s_frame) {
        f32 d_eye = dist(&eye, &s_cam_eye);
        f32 d_center = dist(&center, &s_cam_center);
        if ((d_eye > PROBE_JUMP || d_center > PROBE_JUMP) && log_line(NULL, "camera")) {
            fprintf(s_log, " eye (%.1f %.1f %.1f)->(%.1f %.1f %.1f) center (%.1f %.1f %.1f)->(%.1f %.1f %.1f)",
                    s_cam_eye.x, s_cam_eye.y, s_cam_eye.z, eye.x, eye.y, eye.z, s_cam_center.x, s_cam_center.y,
                    s_cam_center.z, center.x, center.y, center.z);
            log_end();
        }
    }
    s_cam_eye = eye;
    s_cam_center = center;
    s_cam_frame = s_frame;
}

void pc_actor_probe_begin(ACTOR* actor, GAME_PLAY* play) {
    ProbeActor* a;
    u32 frame = play->game_frame + 1;

    if (s_log == NULL) {
        return;
    }
    if (frame != s_frame || play != s_play) {
        // new play or its frame counter restarted: old entries would look alive and block slots
        if (play != s_play || frame < s_frame) {
            memset(s_actors, 0, sizeof(s_actors));
            memset(s_mtxs, 0, sizeof(s_mtxs));
            s_cam_frame = 0;
        }
        s_frame = frame;
        s_play = play;
        check_camera(play);
    }

    a = find_actor(actor);
    s_cur = a;
    s_cur_has_mtx = 0;
    if (a == NULL) {
        return;
    }
    if (a->pos_frame + 1 == s_frame) {
        f32 d = dist(&actor->world.position, &a->pos);
        if (d > PROBE_JUMP && log_line(a, "pos")) {
            fprintf(s_log, " moved %.1f (%.1f %.1f %.1f)->(%.1f %.1f %.1f)", d, a->pos.x, a->pos.y, a->pos.z,
                    actor->world.position.x, actor->world.position.y, actor->world.position.z);
            log_end();
        }
    }
    a->pos = actor->world.position;
    a->pos_frame = s_frame;
}

// Same draw matrix (by draw order) moved far while the first one didn't: a joint or held item flew.
// Only when the actor drew as many matrices as last frame, so the order lines up.
static void check_joints(ProbeActor* a) {
    int worst = -1;
    f32 worst_d = PROBE_JUMP;
    int i;

    if (a->joint_frame + 1 == s_frame && a->joint_n == a->cur_n && a->cur_n > 1 &&
        dist(&a->cur[0], &a->joints[0]) <= PROBE_JUMP) { // root jumps are "draw" lines
        for (i = 1; i < a->cur_n; i++) {
            f32 d = dist(&a->cur[i], &a->joints[i]);
            if (d > worst_d) {
                worst_d = d;
                worst = i;
            }
        }
        if (worst >= 0 && log_line(a, "joint")) {
            fprintf(s_log, " mtx %d of %d moved %.1f (%.1f %.1f %.1f)->(%.1f %.1f %.1f) pos (%.1f %.1f %.1f)", worst,
                    a->cur_n, worst_d, a->joints[worst].x, a->joints[worst].y, a->joints[worst].z, a->cur[worst].x,
                    a->cur[worst].y, a->cur[worst].z, a->actor->world.position.x, a->actor->world.position.y,
                    a->actor->world.position.z);
            log_end();
        }
    }
    memcpy(a->joints, a->cur, sizeof(a->joints));
    a->joint_n = a->cur_n;
    a->joint_frame = s_frame;
}

void pc_actor_probe_end(void) {
    ProbeActor* a = s_cur;

    s_cur = NULL;
    if (a != NULL && s_log != NULL && a->cur_frame == s_frame) {
        check_joints(a);
    }
}

// Inverse of guMtxF2L: element k (row-major) of an N64 s15.16 matrix.
static f32 mtx_elem(const s32* w, int k) {
    s32 hi = w[k / 2];
    s32 lo = w[8 + k / 2];
    s32 v;

    if ((k & 1) == 0) {
        v = (s32)(((u32)hi & 0xFFFF0000u) | (((u32)lo >> 16) & 0xFFFFu));
    } else {
        v = (s32)(((u32)hi << 16) | ((u32)lo & 0xFFFFu));
    }
    return (f32)v / 65536.0f;
}

static ProbeMtx* find_mtx(const void* mtx, int insert) {
    u32 h = ((uintptr_t)mtx >> 3) * 2654435761u;
    int i;

    for (i = 0; i < PROBE_SEARCH; i++) {
        ProbeMtx* e = &s_mtxs[(h + i) & (PROBE_MTXS - 1)];
        if (e->frame != s_frame) {
            // stale entries from older frames count as empty
            return insert ? e : NULL;
        }
        if (e->mtx == mtx) {
            return e;
        }
    }
    return NULL;
}

void pc_actor_probe_mtx(float mf[4][4], const void* mtx) {
    ProbeActor* a = s_cur;
    const s32* w = (const s32*)mtx;
    ProbeMtx* e;
    int k;

    if (a == NULL || s_log == NULL) {
        return;
    }

    // fixed-point copy decodes back to the float matrix
    for (k = 0; k < 16; k++) {
        f32 f = mf[k / 4][k % 4];
        f32 d = mtx_elem(w, k);
        if (fabsf(f - d) > PROBE_FIX_TOL) {
            if (log_line(a, "fix")) {
                fprintf(s_log, " mtx=%p elem %d float %f fixed %f", mtx, k, f, d);
                log_end();
            }
            break;
        }
    }

    xyz_t t;
    t.x = mf[3][0];
    t.y = mf[3][1];
    t.z = mf[3][2];

    // every matrix, in draw order, for check_joints
    if (a->cur_frame != s_frame) {
        a->cur_frame = s_frame;
        a->cur_n = 0;
    }
    if (a->cur_n < PROBE_JOINTS) {
        a->cur[a->cur_n++] = t;
    }

    // first matrix of the actor this frame: compare with last frame's
    if (!s_cur_has_mtx) {
        s_cur_has_mtx = 1;
        if (a->draw_frame + 1 == s_frame) {
            f32 d = dist(&t, &a->draw_pos);
            if (d > PROBE_JUMP && log_line(a, "draw")) {
                fprintf(s_log, " moved %.1f (%.1f %.1f %.1f)->(%.1f %.1f %.1f) pos (%.1f %.1f %.1f)", d,
                        a->draw_pos.x, a->draw_pos.y, a->draw_pos.z, t.x, t.y, t.z, a->actor->world.position.x,
                        a->actor->world.position.y, a->actor->world.position.z);
                log_end();
            }
        }
        a->draw_pos = t;
        a->draw_frame = s_frame;
    }

    // remember it for the gpu check
    e = find_mtx(mtx, 1);
    if (e != NULL) {
        e->mtx = mtx;
        e->frame = s_frame;
        e->owner = a;
        memcpy(e->words, w, sizeof(e->words));
    }
}

void pc_actor_probe_gpu_mtx(const void* mtx) {
    ProbeMtx* e;
    const s32* w = (const s32*)mtx;

    if (s_log == NULL) {
        return;
    }
    e = find_mtx(mtx, 0);
    if (e == NULL || memcmp(e->words, w, sizeof(e->words)) == 0) {
        return;
    }
    if (log_line(NULL, "gpu")) {
        // the owner's slot may be reused by now; print what was recorded only
        fprintf(s_log, " mtx=%p built by actor %p id=0x%04x: translation (%.1f %.1f %.1f)->(%.1f %.1f %.1f)", mtx,
                (void*)e->owner->actor, (u16)e->owner->id, mtx_elem(e->words, 12), mtx_elem(e->words, 13),
                mtx_elem(e->words, 14), mtx_elem(w, 12), mtx_elem(w, 13), mtx_elem(w, 14));
        log_end();
    }
    // log a changed matrix once
    memcpy(e->words, w, sizeof(e->words));
}
