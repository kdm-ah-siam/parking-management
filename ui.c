// ui.c — all raylib drawing and input handling

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "parking.h"
#include "ui.h"

// ── Layout ────────────────────────────────────────────────────────────
#define SCR_W  1200
#define SCR_H   740
#define NAV_W   195
#define TOP_H    95   // title bar 55 + stats bar 40
#define CON_X   NAV_W
#define CON_Y   TOP_H
#define CON_W   (SCR_W - NAV_W)
#define CON_H   (SCR_H - TOP_H)

// ── Colors ────────────────────────────────────────────────────────────
#define C_BG      (Color){ 10,  17,  40, 255}
#define C_PANEL   (Color){ 15,  25,  60, 255}
#define C_HEADER  (Color){  8,  14,  35, 255}
#define C_NAV     (Color){ 12,  20,  48, 255}
#define C_NAV_A   (Color){ 32,  80, 200, 255}   // active nav
#define C_NAV_H   (Color){ 22,  55, 140, 255}   // hovered nav
#define C_CARD    (Color){ 18,  32,  78, 255}
#define C_OCC     (Color){ 65,  10,  10, 255}   // occupied row
#define C_FREE    (Color){  8,  48,  22, 255}   // free row
#define C_INP     (Color){ 18,  32,  72, 255}   // input bg
#define C_INP_A   (Color){ 24,  46, 110, 255}   // active input bg
#define C_BLU     (Color){ 32,  80, 200, 255}
#define C_GRN     (Color){ 22, 140,  50, 255}
#define C_RED     (Color){165,  28,  28, 255}
#define C_YEL     (Color){200, 160,  20, 255}
#define C_BOR     (Color){ 38,  62, 128, 255}
#define C_SUB     (Color){150, 175, 220, 255}
#define C_DIM     (Color){ 80, 100, 148, 255}

// ── Screen enum ───────────────────────────────────────────────────────
typedef enum {
    S_STARTUP, S_VIEW, S_PARK, S_REMOVE,
    S_UPDATE,  S_SEARCH, S_SUMMARY, S_ADD, S_FILE
} Screen;

// ── Text field (simple char buffer) ──────────────────────────────────
#define FLEN 32
typedef struct { char b[FLEN]; } TF;

// ── All UI state in one struct ────────────────────────────────────────
static struct {
    Screen scr;
    int    active;      // which TF id is focused (0 = none)
    int    started;     // 1 after lot is created/loaded
    int    has_file;    // save file exists on disk

    TF  f_start;        // id 1 — startup lot size
    TF  f_plate;        // id 2 — plate (park)
    TF  f_type;         // id 3 — type  (park)
    TF  f_entry;        // id 4 — entry hour
    TF  f_exit;         // id 5 — exit  hour
    TF  f_search;       // id 6 — search query
    TF  f_addslots;     // id 7 — how many to add
    TF  f_uid;          // id 8 — update: slot id
    TF  f_uplate;       // id 9 — update: new plate
    TF  f_utype;        // id 10 — update: new type

    int sel_slot;       // slot id selected from list (-1 = none)
    int sel_size;       // size for park (0/1/2)
    int upd_size;       // size for update (prefilled)
    int search_mode;    // 0=by plate, 1=by id
    int search_idx;     // result index in lot[] (-1 = not found)

    char  msg[128];
    Color msg_col;
    int   scroll;       // view-all table scroll
    int   lscroll;      // slot-list panel scroll
} ui;

// ── Get TF by id ──────────────────────────────────────────────────────
static TF *tf(int id) {
    switch (id) {
        case 1:  return &ui.f_start;
        case 2:  return &ui.f_plate;
        case 3:  return &ui.f_type;
        case 4:  return &ui.f_entry;
        case 5:  return &ui.f_exit;
        case 6:  return &ui.f_search;
        case 7:  return &ui.f_addslots;
        case 8:  return &ui.f_uid;
        case 9:  return &ui.f_uplate;
        case 10: return &ui.f_utype;
        default: return NULL;
    }
}

static void msg_set(const char *s, Color c) {
    strncpy(ui.msg, s, sizeof(ui.msg) - 1);
    ui.msg_col = c;
}

// Slightly brighten a color for hover effects
static Color brighter(Color c) {
    int d = 22;
    return (Color){
        c.r + d > 255 ? 255 : c.r + d,
        c.g + d > 255 ? 255 : c.g + d,
        c.b + d > 255 ? 255 : c.b + d, 255
    };
}

// ── Handle keyboard for the active text field ─────────────────────────
static void handle_keys(void) {
    TF *f = tf(ui.active);
    if (!f) return;
    int k = GetCharPressed();
    while (k > 0) {
        int len = (int)strlen(f->b);
        if (k >= 32 && k <= 125 && len < FLEN - 1) {
            f->b[len] = (char)k;
            f->b[len + 1] = '\0';
        }
        k = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(f->b);
        if (len > 0) f->b[len - 1] = '\0';
    }
    if (IsKeyPressed(KEY_ESCAPE)) ui.active = 0;
}

// ═══════════════════════════════════════════════════════════════════════
//   DRAW HELPERS
// ═══════════════════════════════════════════════════════════════════════

// Button — returns 1 when clicked
static int draw_btn(Rectangle r, const char *text, Color col, Vector2 m) {
    int hover = CheckCollisionPointRec(m, r);
    DrawRectangleRec(r, hover ? brighter(col) : col);
    DrawRectangleLinesEx(r, 1.0f, (Color){255, 255, 255, 20});
    int tw = MeasureText(text, 14);
    DrawText(text, (int)(r.x + (r.width - tw) / 2),
             (int)(r.y + (r.height - 14) / 2), 14, WHITE);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Text input box — sets ui.active when clicked, shows cursor blink
static void draw_input(Rectangle r, TF *f, int id,
                       const char *label, Vector2 m) {
    if (CheckCollisionPointRec(m, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        ui.active = id;
    int active = (ui.active == id);
    if (label) DrawText(label, (int)r.x, (int)r.y - 18, 12, C_SUB);
    DrawRectangleRec(r, active ? C_INP_A : C_INP);
    DrawRectangleLinesEx(r, 1.5f,
        active ? (Color){80, 140, 255, 255} : C_BOR);
    DrawText(f->b, (int)r.x + 8,
             (int)(r.y + (r.height - 14) / 2), 14, WHITE);
    if (active && (int)(GetTime() * 2) % 2) {
        int cx = (int)r.x + 8 + MeasureText(f->b, 14);
        DrawRectangle(cx, (int)(r.y + (r.height - 14) / 2), 2, 14, WHITE);
    }
}

// Section heading with underline accent
static void draw_heading(int x, int y, const char *s) {
    DrawText(s, x, y, 16, C_SUB);
    DrawRectangle(x, y + 22, MeasureText(s, 16), 2, C_BLU);
}

// Stat card
static void draw_card(int x, int y, int w, int h,
                      const char *label, const char *val, Color col) {
    DrawRectangle(x, y, w, h, col);
    DrawRectangleLinesEx((Rectangle){x, y, w, h}, 1.0f,
                         (Color){255, 255, 255, 18});
    DrawText(label, x + 12, y + 9,  11, C_SUB);
    DrawText(val,   x + 12, y + 28, 22, WHITE);
}

// Table column header row
static void draw_tbl_header(int x, int y, int w) {
    DrawRectangle(x, y, w, 28, C_HEADER);
    const char *cols[] = {"Slot","Plate","Type","Size","Entry","Exit","Status"};
    int         offx[] = { 8,    68,    195,   318,   410,   485,   562};
    for (int i = 0; i < 7; i++)
        DrawText(cols[i], x + offx[i], y + 7, 12, C_SUB);
}

// One slot table row
static void draw_tbl_row(int x, int y, int w, int i, int highlight) {
    Color bg = highlight       ? (Color){90, 70, 8, 255} :
               lot[i].occupied ? C_OCC : C_FREE;
    if (i % 2 == 0) {   // alternating shade
        bg.r += 6; bg.g += 6; bg.b += 6;
    }
    DrawRectangle(x, y, w, 28, bg);
    DrawRectangle(x, y + 27, w, 1, (Color){20, 40, 90, 120});

    char tmp[16];
    sprintf(tmp, "%d", lot[i].id);
    DrawText(tmp, x + 8, y + 7, 13, C_SUB);

    if (lot[i].occupied) {
        struct Vehicle *v = &lot[i].v;
        DrawText(v->plate,           x + 68,  y + 7, 13, WHITE);
        DrawText(v->type,            x + 195, y + 7, 12, C_SUB);
        DrawText(SIZE_NAMES[v->size],x + 318, y + 7, 12, C_SUB);
        char es[8]; sprintf(es, "%02d:00", v->entry_hour); DrawText(es, x+410, y+7, 12, C_SUB);
        char xs[8];
        if (v->exit_hour < 0) strcpy(xs, "---");
        else sprintf(xs, "%02d:00", v->exit_hour);
        DrawText(xs, x + 485, y + 7, 12, C_SUB);
        DrawRectangle(x + 562, y + 4, 82, 20, (Color){140, 22, 22, 200});
        DrawText("OCCUPIED", x + 565, y + 8, 11, WHITE);
    } else {
        DrawText("---", x + 68,  y + 7, 13, C_DIM);
        DrawText("---", x + 195, y + 7, 12, C_DIM);
        DrawText("---", x + 318, y + 7, 12, C_DIM);
        DrawText("---", x + 410, y + 7, 12, C_DIM);
        DrawText("---", x + 485, y + 7, 12, C_DIM);
        DrawRectangle(x + 562, y + 4, 50, 20, (Color){18, 110, 45, 200});
        DrawText("FREE", x + 572, y + 8, 11, WHITE);
    }
}

// Clickable slot row used in the left-panel list (park / remove screens)
// Returns 1 if clicked
static int draw_list_row(int x, int y, int w, int i, Vector2 m) {
    Rectangle r = {x, y, w, 38};
    int hover = CheckCollisionPointRec(m, r);
    int sel   = (lot[i].id == ui.sel_slot);
    DrawRectangleRec(r, sel ? C_BLU : hover ? C_NAV_H : C_NAV);
    DrawRectangle(x, y + 37, w, 1, C_BOR);
    char id_s[12]; sprintf(id_s, "Slot  %d", lot[i].id);
    DrawText(id_s, x + 10, y + 5, 14, WHITE);
    if (lot[i].occupied)
        DrawText(lot[i].v.plate, x + 10, y + 22, 11, C_SUB);
    else
        DrawText("Free", x + 10, y + 22, 11, (Color){60, 210, 90, 255});
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Left-side slot list (only_free=1 shows free, only_free=0 shows occupied)
static void draw_slot_list(int x, int y, int w, int h,
                           const char *title, int only_free, Vector2 m) {
    DrawRectangle(x, y, w, h, C_NAV);
    DrawRectangleLinesEx((Rectangle){x, y, w, h}, 1.0f, C_BOR);
    DrawText(title, x + 10, y + 10, 13, C_SUB);
    DrawRectangle(x, y + 30, w, 1, C_BOR);

    int item_h  = 38;
    int visible = (h - 32) / item_h;

    // scroll with mouse wheel when hovering this panel
    Rectangle panel = {x, y + 32, w, h - 32};
    if (CheckCollisionPointRec(m, panel)) {
        float wh = GetMouseWheelMove();
        if (wh != 0) { ui.lscroll -= (int)wh; if (ui.lscroll < 0) ui.lscroll = 0; }
    }

    int count = 0, drawn = 0;
    BeginScissorMode(x, y + 32, w, h - 32);
    for (int i = 0; i < num_slots; i++) {
        if ( only_free &&  lot[i].occupied) continue;
        if (!only_free && !lot[i].occupied) continue;
        if (count++ < ui.lscroll) continue;
        if (drawn >= visible + 1) break;
        int ry = y + 32 + drawn * item_h;
        if (draw_list_row(x, ry, w, i, m)) ui.sel_slot = lot[i].id;
        drawn++;
    }
    EndScissorMode();
}

// ═══════════════════════════════════════════════════════════════════════
//   PERSISTENT TOP BARS
// ═══════════════════════════════════════════════════════════════════════

static void draw_title_bar(void) {
    DrawRectangle(0, 0, SCR_W, 55, C_HEADER);
    DrawText("PARKING MANAGEMENT SYSTEM", 18, 10, 22, WHITE);
    DrawText("Raylib GUI  |  Dynamic Slots  |  Save & Load",
             18, 37, 11, C_DIM);
    DrawRectangle(0, 54, SCR_W, 1, C_BOR);
}

static void draw_stats_bar(void) {
    int occ = 0;
    for (int i = 0; i < num_slots; i++) if (lot[i].occupied) occ++;
    DrawRectangle(0, 55, SCR_W, 40, C_PANEL);

    int bx = NAV_W + 10;
    const char *lbls[] = {"TOTAL","OCCUPIED","AVAILABLE","INCOME"};
    char vs[4][24];
    sprintf(vs[0], "%d",    num_slots);
    sprintf(vs[1], "%d",    occ);
    sprintf(vs[2], "%d",    num_slots - occ);
    sprintf(vs[3], "$%.2f", income);

    for (int i = 0; i < 4; i++) {
        DrawText(lbls[i], bx + i*230,      62, 10, C_DIM);
        DrawText(vs[i],   bx + i*230 + 80, 60, 16, WHITE);
    }
    DrawRectangle(0, 94, SCR_W, 1, C_BOR);
}

static void draw_nav(Vector2 m) {
    DrawRectangle(0, TOP_H, NAV_W, SCR_H - TOP_H, C_NAV);
    DrawRectangle(NAV_W - 1, TOP_H, 1, SCR_H - TOP_H, C_BOR);

    const char *labels[] = {
        "View All", "Park Vehicle", "Remove Vehicle",
        "Update Info", "Search", "Summary",
        "Add Slots", "Save / Load"
    };
    Screen screens[] = {
        S_VIEW, S_PARK, S_REMOVE,
        S_UPDATE, S_SEARCH, S_SUMMARY,
        S_ADD, S_FILE
    };

    for (int i = 0; i < 8; i++) {
        Rectangle r = {5, TOP_H + 8 + i * 52, NAV_W - 10, 44};
        int active = (ui.scr == screens[i]);
        int hover  = CheckCollisionPointRec(m, r);
        if (active) DrawRectangleRec(r, C_NAV_A);
        else if (hover) DrawRectangleRec(r, C_NAV_H);
        if (active) DrawRectangle(5, (int)r.y, 3, 44, (Color){120, 190, 255, 255});
        DrawText(labels[i], 18, (int)r.y + 15, 13, active ? WHITE : C_SUB);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ui.scr     = screens[i];
            ui.active  = 0;
            ui.sel_slot= -1;
            ui.scroll  = 0;
            ui.lscroll = 0;
            msg_set("", WHITE);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: VIEW ALL
// ═══════════════════════════════════════════════════════════════════════

static void draw_view_all(Vector2 m) {
    (void)m;
    int x = CON_X + 10, y = CON_Y + 12, w = CON_W - 20;
    draw_heading(x, y, "All Parking Slots");

    int ty      = y + 34;
    int row_h   = 28;
    int visible = (SCR_H - ty - 24) / row_h;

    // scroll with mouse wheel
    float wh = GetMouseWheelMove();
    if (wh != 0) {
        ui.scroll -= (int)wh;
        if (ui.scroll < 0) ui.scroll = 0;
        int max_s = num_slots - visible;
        if (ui.scroll > max_s) ui.scroll = max_s < 0 ? 0 : max_s;
    }

    draw_tbl_header(x, ty, w);
    int clip_y = ty + 28;
    int clip_h = SCR_H - clip_y - 22;
    BeginScissorMode(x, clip_y, w, clip_h);
    for (int i = ui.scroll; i < num_slots; i++) {
        int ry = clip_y + (i - ui.scroll) * row_h;
        if (ry > SCR_H) break;
        draw_tbl_row(x, ry, w, i, 0);
    }
    EndScissorMode();

    // footer
    char s[64];
    int occ = 0; for (int i = 0; i < num_slots; i++) if (lot[i].occupied) occ++;
    sprintf(s, "%d slots    %d occupied    %d free", num_slots, occ, num_slots - occ);
    DrawText(s, x, SCR_H - 16, 11, C_DIM);
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: PARK VEHICLE
// ═══════════════════════════════════════════════════════════════════════

static void draw_park(Vector2 m) {
    int lw = 210;              // left list width
    int fx = CON_X + lw + 18; // form start x
    int fy = CON_Y + 15;

    draw_slot_list(CON_X + 4, CON_Y + 4, lw, CON_H - 8,
                   "Free Slots", 1, m);
    DrawRectangle(fx - 9, CON_Y, 1, CON_H, C_BOR);

    draw_heading(fx, fy, "Park a Vehicle");  fy += 36;

    // Selected slot
    DrawText("Selected Slot :", fx, fy, 13, C_SUB);
    char sel_s[24];
    if (ui.sel_slot > 0) sprintf(sel_s, "Slot  %d", ui.sel_slot);
    else                 strcpy(sel_s, "click list →");
    DrawText(sel_s, fx + 130, fy, 13,
             ui.sel_slot > 0 ? WHITE : C_DIM);
    fy += 32;

    draw_input((Rectangle){fx, fy + 18, 270, 32},
               &ui.f_plate, 2, "Plate Number", m);  fy += 68;
    draw_input((Rectangle){fx, fy + 18, 270, 32},
               &ui.f_type,  3, "Vehicle Type  (Car / Truck / Motorcycle)", m); fy += 68;

    // Size selector
    DrawText("Vehicle Size", fx, fy, 12, C_SUB);  fy += 18;
    const char *sz[] = {"Small","Medium","Big"};
    Color sz_c[] = {C_GRN, C_BLU, C_RED};
    for (int s = 0; s < 3; s++) {
        Rectangle rb = {fx + s * 95, fy, 88, 30};
        Color bg = (ui.sel_size == s) ? sz_c[s] : C_INP;
        if (draw_btn(rb, sz[s], bg, m)) ui.sel_size = s;
        if (ui.sel_size == s)
            DrawRectangleLinesEx(rb, 2.0f, (Color){255,255,255,60});
    }
    fy += 44;

    draw_input((Rectangle){fx, fy + 18, 100, 32},
               &ui.f_entry, 4, "Entry Hour  (0-23)", m);  fy += 68;

    if (draw_btn((Rectangle){fx, fy, 210, 42}, "PARK VEHICLE", C_GRN, m)) {
        char err[128] = "";
        if (ui.sel_slot <= 0) {
            msg_set("Select a free slot from the list", C_YEL);
        } else if (park_car(ui.sel_slot, ui.f_plate.b, ui.f_type.b,
                            ui.sel_size, atoi(ui.f_entry.b), err)) {
            char ok[64]; sprintf(ok, "Parked in Slot %d!", ui.sel_slot);
            msg_set(ok, (Color){60, 220, 100, 255});
            memset(&ui.f_plate, 0, sizeof(TF));
            memset(&ui.f_type,  0, sizeof(TF));
            memset(&ui.f_entry, 0, sizeof(TF));
            ui.sel_slot = -1;
            ui.active   = 0;
        } else {
            msg_set(err, C_RED);
        }
    }
    fy += 52;
    DrawText(ui.msg, fx, fy, 13, ui.msg_col);
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: REMOVE VEHICLE
// ═══════════════════════════════════════════════════════════════════════

static void draw_remove(Vector2 m) {
    int lw = 210;
    int fx = CON_X + lw + 18;
    int fy = CON_Y + 15;

    draw_slot_list(CON_X + 4, CON_Y + 4, lw, CON_H - 8,
                   "Occupied Slots", 0, m);
    DrawRectangle(fx - 9, CON_Y, 1, CON_H, C_BOR);

    draw_heading(fx, fy, "Remove a Vehicle");  fy += 36;

    if (ui.sel_slot <= 0) {
        DrawText("Select an occupied slot from the list.", fx, fy + 20, 14, C_DIM);
        return;
    }

    int idx = find_slot(ui.sel_slot);
    if (idx < 0 || !lot[idx].occupied) {
        DrawText("Slot is now empty.", fx, fy + 20, 14, C_DIM);
        ui.sel_slot = -1;
        return;
    }

    struct Vehicle *v = &lot[idx].v;
    DrawText("Parked vehicle:", fx, fy, 13, C_SUB);  fy += 22;
    char line[48];
    sprintf(line, "Plate : %s", v->plate); DrawText(line, fx, fy, 14, WHITE); fy += 22;
    sprintf(line, "Type  : %s", v->type);  DrawText(line, fx, fy, 14, WHITE); fy += 22;
    sprintf(line, "Size  : %s", SIZE_NAMES[v->size]); DrawText(line, fx, fy, 14, WHITE); fy += 22;
    sprintf(line, "Entry : %02d:00", v->entry_hour);  DrawText(line, fx, fy, 14, WHITE); fy += 30;

    draw_input((Rectangle){fx, fy + 18, 110, 32},
               &ui.f_exit, 5, "Exit Hour  (0-23)", m);  fy += 68;

    // Live fee preview
    if (ui.f_exit.b[0]) {
        int exit_h = atoi(ui.f_exit.b);
        int hours  = exit_h - v->entry_hour;
        if (hours <= 0) hours = 1;
        double fee = calc_fee(v->size, hours);
        char preview[64];
        sprintf(preview, "Fee preview:  %d hr  x  $%.0f  =  $%.2f",
                hours, (v->size == 0 ? 1.0 : v->size == 1 ? 2.0 : 3.0), fee);
        DrawText(preview, fx, fy, 13, (Color){255, 200, 50, 255});
        fy += 28;
    }

    if (draw_btn((Rectangle){fx, fy, 225, 42}, "REMOVE VEHICLE", C_RED, m)) {
        char err[128] = "";
        double fee; int hours;
        if (remove_car(ui.sel_slot, atoi(ui.f_exit.b), &fee, &hours, err)) {
            char ok[80];
            sprintf(ok, "Removed!  %d hr    Fee: $%.2f    Total income: $%.2f",
                    hours, fee, income);
            msg_set(ok, (Color){60, 220, 100, 255});
            memset(&ui.f_exit, 0, sizeof(TF));
            ui.sel_slot = -1;
            ui.active   = 0;
        } else {
            msg_set(err, C_RED);
        }
    }
    fy += 52;
    DrawText(ui.msg, fx, fy, 13, ui.msg_col);
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: UPDATE INFO
// ═══════════════════════════════════════════════════════════════════════

static void draw_update(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 15;
    draw_heading(x, y, "Update Vehicle Info");  y += 40;

    draw_input((Rectangle){x, y + 18, 150, 32}, &ui.f_uid, 8, "Slot ID", m);
    if (draw_btn((Rectangle){x + 160, y + 18, 90, 32}, "Load", C_BLU, m)) {
        int id  = atoi(ui.f_uid.b);
        int idx = find_slot(id);
        if (idx < 0)             msg_set("Slot not found",   C_RED);
        else if(!lot[idx].occupied) msg_set("Slot is empty", C_RED);
        else {
            strncpy(ui.f_uplate.b, lot[idx].v.plate, FLEN - 1);
            strncpy(ui.f_utype.b,  lot[idx].v.type,  FLEN - 1);
            ui.upd_size = lot[idx].v.size;
            ui.sel_slot = id;
            msg_set("", WHITE);
        }
    }
    y += 68;

    if (ui.sel_slot <= 0) {
        DrawText("Enter a slot ID and click Load.", x, y, 14, C_DIM);
        return;
    }

    DrawText("Edit fields below (clear a field to keep its current value).",
             x, y, 12, C_DIM);  y += 28;

    draw_input((Rectangle){x, y + 18, 270, 32}, &ui.f_uplate, 9, "Plate", m); y += 68;
    draw_input((Rectangle){x, y + 18, 270, 32}, &ui.f_utype, 10, "Type",  m); y += 68;

    DrawText("Size", x, y, 12, C_SUB);  y += 18;
    const char *sz[] = {"Small","Medium","Big"};
    Color sz_c[] = {C_GRN, C_BLU, C_RED};
    for (int s = 0; s < 3; s++) {
        Rectangle rb = {x + s * 95, y, 88, 30};
        Color bg = (ui.upd_size == s) ? sz_c[s] : C_INP;
        if (draw_btn(rb, sz[s], bg, m)) ui.upd_size = s;
        if (ui.upd_size == s) DrawRectangleLinesEx(rb, 2.0f, (Color){255,255,255,60});
    }
    y += 48;

    if (draw_btn((Rectangle){x, y, 200, 42}, "SAVE CHANGES", C_GRN, m)) {
        char err[128] = "";
        if (update_car(ui.sel_slot,
                       ui.f_uplate.b, ui.f_utype.b, ui.upd_size, err)) {
            msg_set("Updated successfully!", (Color){60, 220, 100, 255});
            ui.sel_slot = -1;
            memset(&ui.f_uid,    0, sizeof(TF));
            memset(&ui.f_uplate, 0, sizeof(TF));
            memset(&ui.f_utype,  0, sizeof(TF));
            ui.active = 0;
        } else {
            msg_set(err, C_RED);
        }
    }
    y += 52;
    DrawText(ui.msg, x, y, 13, ui.msg_col);
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: SEARCH
// ═══════════════════════════════════════════════════════════════════════

static void draw_search(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 15;
    draw_heading(x, y, "Search");  y += 40;

    // Mode toggle
    Color a_col = C_BLU, i_col = C_INP;
    if (draw_btn((Rectangle){x,       y, 150, 34},
                 "By Plate",
                 ui.search_mode == 0 ? a_col : i_col, m)) {
        ui.search_mode = 0; ui.search_idx = -1;
    }
    if (draw_btn((Rectangle){x + 158, y, 150, 34},
                 "By Slot ID",
                 ui.search_mode == 1 ? a_col : i_col, m)) {
        ui.search_mode = 1; ui.search_idx = -1;
    }
    y += 50;

    draw_input((Rectangle){x, y + 18, 300, 32}, &ui.f_search, 6,
               ui.search_mode == 0 ? "Plate Number" : "Slot ID", m);
    y += 68;

    if (draw_btn((Rectangle){x, y, 130, 36}, "SEARCH", C_BLU, m)) {
        if (ui.search_mode == 0)
            ui.search_idx = find_plate(ui.f_search.b);
        else
            ui.search_idx = find_slot(atoi(ui.f_search.b));

        if (ui.search_idx < 0) msg_set("Not found.", C_RED);
        else                   msg_set("", WHITE);
    }
    y += 48;

    DrawText(ui.msg, x, y, 13, ui.msg_col);  y += 24;

    if (ui.search_idx >= 0) {
        draw_tbl_header(x, y, CON_W - 40);
        draw_tbl_row(x, y + 28, CON_W - 40, ui.search_idx, 1);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: SUMMARY
// ═══════════════════════════════════════════════════════════════════════

static void draw_summary(Vector2 m) {
    (void)m;
    int x = CON_X + 20, y = CON_Y + 20;
    draw_heading(x, y, "Summary");  y += 44;

    int occ = 0, sm = 0, med = 0, big = 0;
    for (int i = 0; i < num_slots; i++) {
        if (!lot[i].occupied) continue;
        occ++;
        if      (lot[i].v.size == 0) sm++;
        else if (lot[i].v.size == 1) med++;
        else                         big++;
    }

    char s[32];
    sprintf(s, "%d", num_slots);
    draw_card(x,       y, 195, 76, "TOTAL SLOTS",  s, C_CARD);
    sprintf(s, "%d", occ);
    draw_card(x + 205, y, 195, 76, "OCCUPIED",     s, C_RED);
    sprintf(s, "%d", num_slots - occ);
    draw_card(x + 410, y, 195, 76, "AVAILABLE",    s, C_GRN);
    sprintf(s, "$%.2f", income);
    draw_card(x + 615, y, 210, 76, "TOTAL INCOME", s, (Color){130, 85, 15, 255});
    y += 96;

    DrawText("Breakdown of occupied slots by size:", x, y, 13, C_SUB); y += 26;
    sprintf(s, "Small  : %d", sm);  DrawText(s, x, y, 14, WHITE); y += 22;
    sprintf(s, "Medium : %d", med); DrawText(s, x, y, 14, WHITE); y += 22;
    sprintf(s, "Big    : %d", big); DrawText(s, x, y, 14, WHITE); y += 32;

    DrawText("Fee rates per hour:", x, y, 13, C_SUB); y += 24;
    DrawText("Small  : $1.00 / hr", x, y, 13, C_DIM); y += 20;
    DrawText("Medium : $2.00 / hr", x, y, 13, C_DIM); y += 20;
    DrawText("Big    : $3.00 / hr", x, y, 13, C_DIM);
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: ADD SLOTS
// ═══════════════════════════════════════════════════════════════════════

static void draw_add_slots(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 20;
    draw_heading(x, y, "Add New Parking Slots");  y += 50;

    char cur[40];
    sprintf(cur, "Current slot count:  %d", num_slots);
    DrawText(cur, x, y, 15, C_SUB);  y += 42;

    draw_input((Rectangle){x, y + 18, 170, 36}, &ui.f_addslots, 7,
               "How many slots to add?", m);  y += 72;

    if (draw_btn((Rectangle){x, y, 170, 42}, "ADD SLOTS", C_GRN, m)) {
        int n = atoi(ui.f_addslots.b);
        if (n <= 0) {
            msg_set("Enter a positive number", C_YEL);
        } else {
            add_slots_n(n);
            char ok[48]; sprintf(ok, "Added %d.  Total: %d slots.", n, num_slots);
            msg_set(ok, (Color){60, 220, 100, 255});
            memset(&ui.f_addslots, 0, sizeof(TF));
        }
    }
    y += 54;
    DrawText(ui.msg, x, y, 13, ui.msg_col);
}

// ═══════════════════════════════════════════════════════════════════════
//   SCREEN: SAVE / LOAD
// ═══════════════════════════════════════════════════════════════════════

static void draw_file(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 20;
    draw_heading(x, y, "Save & Load Data");  y += 50;

    DrawText("File:  parking_data.txt", x, y, 13, C_DIM);  y += 34;

    if (draw_btn((Rectangle){x, y, 290, 52}, "SAVE DATA TO FILE", C_GRN, m)) {
        char err[128] = "";
        if (save_file(err)) msg_set("Saved to parking_data.txt", (Color){60,220,100,255});
        else                msg_set(err, C_RED);
    }
    y += 68;

    if (draw_btn((Rectangle){x, y, 290, 52}, "LOAD DATA FROM FILE", C_BLU, m)) {
        char err[128] = "";
        if (load_file(err)) {
            msg_set("Data loaded successfully!", (Color){60,220,100,255});
            ui.scroll = ui.lscroll = 0;
        } else {
            msg_set(err, C_RED);
        }
    }
    y += 68;
    DrawText(ui.msg, x, y, 13, ui.msg_col);
}

// ═══════════════════════════════════════════════════════════════════════
//   STARTUP OVERLAY
// ═══════════════════════════════════════════════════════════════════════

static void draw_startup(Vector2 m) {
    DrawRectangle(0, 0, SCR_W, SCR_H, (Color){0, 0, 0, 190});

    int bw = 500, bh = ui.has_file ? 265 : 195;
    int bx = (SCR_W - bw) / 2, by = (SCR_H - bh) / 2;

    DrawRectangle(bx, by, bw, bh, C_PANEL);
    DrawRectangleLinesEx((Rectangle){bx, by, bw, bh}, 2.0f, C_BLU);
    DrawText("PARKING MANAGEMENT SYSTEM", bx + 20, by + 16, 18, WHITE);
    DrawRectangle(bx + 20, by + 42, bw - 40, 1, C_BOR);

    if (ui.has_file) {
        DrawText("Save file found:  parking_data.txt", bx+20, by+54, 13, C_SUB);
        if (draw_btn((Rectangle){bx+20, by+84, 220, 44},
                     "LOAD SAVED DATA", C_GRN, m)) {
            char err[128] = "";
            if (load_file(err)) { ui.started = 1; ui.scr = S_VIEW; }
            else                  msg_set(err, C_RED);
        }
        DrawText("— or start a new lot —", bx + 150, by + 140, 12, C_DIM);
        draw_input((Rectangle){bx+20, by+164, 165, 34}, &ui.f_start, 1, NULL, m);
        if (draw_btn((Rectangle){bx+194, by+164, 100, 34}, "CREATE", C_BLU, m)
            || IsKeyPressed(KEY_ENTER)) {
            int n = atoi(ui.f_start.b);
            if (n > 0) { init_lot(n); ui.started = 1; ui.scr = S_VIEW; }
            else msg_set("Enter a valid size", C_RED);
        }
    } else {
        DrawText("Enter the parking lot size to begin:", bx+20, by+54, 13, C_SUB);
        draw_input((Rectangle){bx+20, by+86, 200, 36}, &ui.f_start, 1, NULL, m);
        if (draw_btn((Rectangle){bx+228, by+86, 120, 36}, "CREATE", C_BLU, m)
            || IsKeyPressed(KEY_ENTER)) {
            int n = atoi(ui.f_start.b);
            if (n > 0) { init_lot(n); ui.started = 1; ui.scr = S_VIEW; }
            else msg_set("Enter a positive number", C_RED);
        }
    }
    DrawText(ui.msg, bx + 20, by + bh - 24, 12, ui.msg_col);
}

// ═══════════════════════════════════════════════════════════════════════
//   PUBLIC API
// ═══════════════════════════════════════════════════════════════════════

void ui_init(void) {
    memset(&ui, 0, sizeof(ui));
    ui.scr        = S_STARTUP;
    ui.active     = 1;   // focus startup input
    ui.sel_slot   = -1;
    ui.search_idx = -1;
    ui.upd_size   = 0;
    ui.msg_col    = WHITE;

    FILE *f = fopen(DATA_FILE, "r");
    if (f) { fclose(f); ui.has_file = 1; }
}

void ui_update(void) {
    handle_keys();
}

void ui_draw(void) {
    Vector2 m = GetMousePosition();

    if (!ui.started) {
        draw_title_bar();
        draw_startup(m);
        return;
    }

    draw_title_bar();
    draw_stats_bar();
    draw_nav(m);
    DrawRectangle(CON_X, CON_Y, CON_W, CON_H, C_BG);

    switch (ui.scr) {
        case S_VIEW:    draw_view_all(m);   break;
        case S_PARK:    draw_park(m);       break;
        case S_REMOVE:  draw_remove(m);     break;
        case S_UPDATE:  draw_update(m);     break;
        case S_SEARCH:  draw_search(m);     break;
        case S_SUMMARY: draw_summary(m);    break;
        case S_ADD:     draw_add_slots(m);  break;
        case S_FILE:    draw_file(m);       break;
        default: break;
    }
}
