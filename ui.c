// ui.c — all raylib drawing and input handling

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "raylib.h"
#include "parking.h"
#include "ui.h"

#define SCR_W  1200
#define SCR_H   740
#define NAV_W   195
#define TOP_H    95
#define CON_X   NAV_W
#define CON_Y   TOP_H
#define CON_W   (SCR_W - NAV_W)
#define CON_H   (SCR_H - TOP_H)

#define C_BG      (Color){ 10,  17,  40, 255}
#define C_PANEL   (Color){ 15,  25,  60, 255}
#define C_HEADER  (Color){  8,  14,  35, 255}
#define C_NAV     (Color){ 12,  20,  48, 255}
#define C_NAV_A   (Color){ 32,  80, 200, 255}
#define C_NAV_H   (Color){ 22,  55, 140, 255}
#define C_CARD    (Color){ 18,  32,  78, 255}
#define C_OCC     (Color){ 65,  10,  10, 255}
#define C_FREE    (Color){  8,  48,  22, 255}
#define C_INP     (Color){ 18,  32,  72, 255}
#define C_INP_A   (Color){ 24,  46, 110, 255}
#define C_BLU     (Color){ 32,  80, 200, 255}
#define C_GRN     (Color){ 22, 140,  50, 255}
#define C_RED     (Color){165,  28,  28, 255}
#define C_YEL     (Color){200, 160,  20, 255}
#define C_BOR     (Color){ 38,  62, 128, 255}
#define C_SUB     (Color){150, 175, 220, 255}
#define C_DIM     (Color){ 80, 100, 148, 255}

typedef enum {
    S_STARTUP, S_VIEW, S_PARK, S_REMOVE,
    S_UPDATE,  S_SEARCH, S_SUMMARY, S_REPORTS, S_SETTINGS, S_ADD, S_FILE,
    S_HISTORY, S_ROLE_SELECT, S_ADMIN_LOGIN
} Screen;

#define FLEN 32
typedef struct { char b[FLEN]; } TF;

static struct {
    Screen scr;
    int    active, started, has_file;
    TF  f_start, f_plate, f_type, f_entry, f_exit;
    TF  f_search, f_addslots, f_uid, f_uplate, f_utype;
    TF  f_username, f_password;
    TF  f_rep_day, f_rep_month, f_rep_year;
    TF  f_set_user, f_set_pass, f_set_r0, f_set_r1, f_set_r2;
    TF  f_hist_plate;
    int sel_slot, sel_size, upd_size, search_mode, search_idx;
    char  msg[128];
    Color msg_col;
    int   scroll, lscroll;
    int   role;
    char  admin_user[64], admin_pass[64];
    int   report_mode;
    int   confirm_remove, confirm_slot_id;
} ui;

static TF *tf(int id) {
    TF *fields[] = {
        NULL,
        &ui.f_start, &ui.f_plate, &ui.f_type, &ui.f_entry, &ui.f_exit,
        &ui.f_search, &ui.f_addslots, &ui.f_uid, &ui.f_uplate, &ui.f_utype,
        &ui.f_username, &ui.f_password,
        &ui.f_rep_day, &ui.f_rep_month, &ui.f_rep_year,
        &ui.f_set_user, &ui.f_set_pass, &ui.f_set_r0, &ui.f_set_r1, &ui.f_set_r2,
        &ui.f_hist_plate
    };
    return (id >= 1 && id <= 21) ? fields[id] : NULL;
}

static int infer_size(const char *type) {
    if (!type || !type[0]) return -1;
    char low[MAX_TYPE] = {0};
    for (int i = 0; type[i] && i < MAX_TYPE - 1; i++)
        low[i] = tolower((unsigned char)type[i]);
    if (strstr(low,"motorcycle") || strstr(low,"bike") ||
        strstr(low,"moto")       || strstr(low,"scooter")) return 0;
    if (strstr(low,"truck") || strstr(low,"bus") || strstr(low,"lorry")) return 2;
    if (strstr(low,"car")   || strstr(low,"sedan") ||
        strstr(low,"suv")   || strstr(low,"van"))   return 1;
    return -1;
}

static void msg_set(const char *s, Color c) {
    strncpy(ui.msg, s, sizeof(ui.msg) - 1);
    ui.msg_col = c;
}

static Color brighter(Color c) {
    c.r = c.r + 22 > 255 ? 255 : c.r + 22;
    c.g = c.g + 22 > 255 ? 255 : c.g + 22;
    c.b = c.b + 22 > 255 ? 255 : c.b + 22;
    return c;
}

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

static void scroll_clamp(int *s, int count, int visible,
                         Rectangle panel, Vector2 m) {
    if (!CheckCollisionPointRec(m, panel)) return;
    float wh = GetMouseWheelMove();
    if (wh == 0) return;
    *s -= (int)wh;
    if (*s < 0) *s = 0;
    int max_s = count - visible;
    if (max_s < 0) max_s = 0;
    if (*s > max_s) *s = max_s;
}

// ── draw helpers ──────────────────────────────────────────────────────

static int draw_btn(Rectangle r, const char *text, Color col, Vector2 m) {
    int hover = CheckCollisionPointRec(m, r);
    DrawRectangleRec(r, hover ? brighter(col) : col);
    DrawRectangleLinesEx(r, 1.0f, (Color){255, 255, 255, 20});
    int tw = MeasureText(text, 14);
    DrawText(text, (int)(r.x + (r.width - tw) / 2),
             (int)(r.y + (r.height - 14) / 2), 14, WHITE);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void draw_input(Rectangle r, TF *f, int id,
                       const char *label, Vector2 m, int masked) {
    if (CheckCollisionPointRec(m, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        ui.active = id;
    int active = (ui.active == id);
    if (label) DrawText(label, (int)r.x, (int)r.y - 18, 12, C_SUB);
    if (active) {
        DrawRectangleRec(r, C_INP_A);
        DrawRectangleLinesEx(r, 1.5f, (Color){80, 140, 255, 255});
    } else {
        DrawRectangleRec(r, C_INP);
        DrawRectangleLinesEx(r, 1.5f, C_BOR);
    }
    char stars[FLEN];
    const char *display;
    if (masked) {
        int len = (int)strlen(f->b);
        for (int i = 0; i < len; i++) stars[i] = '*';
        stars[len] = '\0';
        display = stars;
    } else {
        display = f->b;
    }
    DrawText(display, (int)r.x + 8, (int)(r.y + (r.height - 14) / 2), 14, WHITE);
    if (active && (int)(GetTime() * 2) % 2) {
        int cx = (int)r.x + 8 + MeasureText(display, 14);
        DrawRectangle(cx, (int)(r.y + (r.height - 14) / 2), 2, 14, WHITE);
    }
}

static void draw_heading(int x, int y, const char *s) {
    DrawText(s, x, y, 16, C_SUB);
    DrawRectangle(x, y + 22, MeasureText(s, 16), 2, C_BLU);
}

static void draw_card(int x, int y, int w, int h,
                      const char *label, const char *val, Color col) {
    DrawRectangle(x, y, w, h, col);
    DrawRectangleLinesEx((Rectangle){x, y, w, h}, 1.0f, (Color){255, 255, 255, 18});
    DrawText(label, x + 12, y + 9,  11, C_SUB);
    DrawText(val,   x + 12, y + 28, 22, WHITE);
}

static void draw_tbl_header(int x, int y, int w) {
    DrawRectangle(x, y, w, 28, C_HEADER);
    const char *cols[] = {"Slot","Plate","Type","Size","Entry","Exit","Status"};
    int         offx[] = {8, 68, 195, 318, 410, 485, 562};
    for (int i = 0; i < 7; i++)
        DrawText(cols[i], x + offx[i], y + 7, 12, C_SUB);
}

static void draw_tbl_row(int x, int y, int w, int i, int highlight) {
    Color bg = highlight ? (Color){90, 70, 8, 255}
             : lot[i].occupied ? C_OCC : C_FREE;
    if (i % 2 == 0) { bg.r += 6; bg.g += 6; bg.b += 6; }
    DrawRectangle(x, y, w, 28, bg);
    DrawRectangle(x, y + 27, w, 1, (Color){20, 40, 90, 120});

    char tmp[16];
    sprintf(tmp, "%d", lot[i].id);
    DrawText(tmp, x + 8, y + 7, 13, C_SUB);

    if (lot[i].occupied) {
        struct Vehicle *v = &lot[i].v;
        DrawText(v->plate,            x + 68,  y + 7, 13, WHITE);
        DrawText(v->type,             x + 195, y + 7, 12, C_SUB);
        DrawText(SIZE_NAMES[v->size], x + 318, y + 7, 12, C_SUB);
        char es[8]; sprintf(es, "%02d:00", v->entry_hour);
        DrawText(es, x + 410, y + 7, 12, C_SUB);
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

static int draw_list_row(int x, int y, int w, int i, Vector2 m) {
    Rectangle r = {x, y, w, 38};
    int hover = CheckCollisionPointRec(m, r);
    int sel   = (lot[i].id == ui.sel_slot);
    DrawRectangleRec(r, sel ? C_BLU : hover ? C_NAV_H : C_NAV);
    DrawRectangle(x, y + 37, w, 1, C_BOR);
    char id_s[12]; sprintf(id_s, "Slot  %d", lot[i].id);
    DrawText(id_s, x + 10, y + 5, 14, WHITE);
    if (lot[i].occupied) DrawText(lot[i].v.plate, x + 10, y + 22, 11, C_SUB);
    else                 DrawText("Free",          x + 10, y + 22, 11, (Color){60, 210, 90, 255});
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void draw_slot_list(int x, int y, int w, int h,
                           const char *title, int only_free, Vector2 m) {
    DrawRectangle(x, y, w, h, C_NAV);
    DrawRectangleLinesEx((Rectangle){x, y, w, h}, 1.0f, C_BOR);
    DrawText(title, x + 10, y + 10, 13, C_SUB);
    DrawRectangle(x, y + 30, w, 1, C_BOR);

    int item_h  = 38;
    int visible = (h - 32) / item_h;
    scroll_clamp(&ui.lscroll, num_slots, visible,
                 (Rectangle){x, y + 32, w, h - 32}, m);

    int count = 0, drawn = 0;
    BeginScissorMode(x, y + 32, w, h - 32);
    for (int i = 0; i < num_slots; i++) {
        if (only_free == 1 && lot[i].occupied == 1) continue;
        if (only_free == 0 && lot[i].occupied == 0) continue;
        if (count < ui.lscroll) { count++; continue; }
        count++;
        if (drawn >= visible + 1) break;
        if (draw_list_row(x, y + 32 + drawn * item_h, w, i, m))
            ui.sel_slot = lot[i].id;
        drawn++;
    }
    EndScissorMode();
}

// ── persistent top bars ───────────────────────────────────────────────

static void draw_title_bar(void) {
    DrawRectangle(0, 0, SCR_W, 55, C_HEADER);
    DrawText("PARKING MANAGEMENT SYSTEM", 18, 10, 22, WHITE);
    DrawText("Raylib GUI  |  Dynamic Slots  |  Save & Load", 18, 37, 11, C_DIM);
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
        "Reports", "Settings", "Add Slots", "Save / Load", "Veh. History"
    };
    Screen screens[] = {
        S_VIEW, S_PARK, S_REMOVE,
        S_UPDATE, S_SEARCH, S_SUMMARY,
        S_REPORTS, S_SETTINGS, S_ADD, S_FILE, S_HISTORY
    };

    int show_count = (ui.role == 2) ? 11 : 6;
    for (int i = 0; i < show_count; i++) {
        Rectangle r = {5, TOP_H + 8 + i * 52, NAV_W - 10, 44};
        int active = (ui.scr == screens[i]);
        int hover  = CheckCollisionPointRec(m, r);
        if (active) {
            DrawRectangleRec(r, C_NAV_A);
            DrawRectangle(5, (int)r.y, 3, 44, (Color){120, 190, 255, 255});
        } else if (hover) {
            DrawRectangleRec(r, C_NAV_H);
        }
        DrawText(labels[i], 18, (int)r.y + 15, 13, active ? WHITE : C_SUB);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ui.scr = screens[i];
            ui.active = 0; ui.sel_slot = -1;
            ui.scroll = 0; ui.lscroll = 0;
            ui.confirm_remove = 0;
            msg_set("", WHITE);
        }
    }

    if (draw_btn((Rectangle){5, SCR_H - 52, NAV_W - 10, 44}, "LOGOUT", C_RED, m)) {
        ui.role = 0; ui.started = 0;
        ui.scr = S_ROLE_SELECT;
        ui.active = 0; ui.sel_slot = -1;
        ui.scroll = 0; ui.lscroll = 0;
        msg_set("", WHITE);
    }
}

// ── SCREEN: VIEW ALL ─────────────────────────────────────────────────

static void draw_view_all(Vector2 m) {
    (void)m;
    int x = CON_X + 10, y = CON_Y + 12, w = CON_W - 20;
    draw_heading(x, y, "All Parking Slots");

    int ty      = y + 34;
    int row_h   = 28;
    int visible = (SCR_H - ty - 24) / row_h;

    float wh = GetMouseWheelMove();
    if (wh != 0) {
        ui.scroll -= (int)wh;
        if (ui.scroll < 0) ui.scroll = 0;
        int max_s = num_slots - visible;
        if (max_s < 0) max_s = 0;
        if (ui.scroll > max_s) ui.scroll = max_s;
    }

    draw_tbl_header(x, ty, w);
    int clip_y = ty + 28, clip_h = SCR_H - clip_y - 22;
    BeginScissorMode(x, clip_y, w, clip_h);
    for (int i = ui.scroll; i < num_slots; i++) {
        int ry = clip_y + (i - ui.scroll) * row_h;
        if (ry > SCR_H) break;
        draw_tbl_row(x, ry, w, i, 0);
    }
    EndScissorMode();

    int occ = 0;
    for (int i = 0; i < num_slots; i++) if (lot[i].occupied) occ++;
    char s[64];
    sprintf(s, "%d slots    %d occupied    %d free", num_slots, occ, num_slots - occ);
    DrawText(s, x, SCR_H - 16, 11, C_DIM);
}

// ── SCREEN: PARK VEHICLE ─────────────────────────────────────────────

static void draw_park(Vector2 m) {
    int lw = 210, fx = CON_X + lw + 18, fy = CON_Y + 15;
    draw_slot_list(CON_X + 4, CON_Y + 4, lw, CON_H - 8, "Free Slots", 1, m);
    DrawRectangle(fx - 9, CON_Y, 1, CON_H, C_BOR);
    draw_heading(fx, fy, "Park a Vehicle");  fy += 36;

    DrawText("Selected Slot :", fx, fy, 13, C_SUB);
    char sel_s[24];
    if (ui.sel_slot > 0) sprintf(sel_s, "Slot  %d", ui.sel_slot);
    else                 strcpy(sel_s, "click list \xe2\x86\x92");
    DrawText(sel_s, fx + 130, fy, 13, ui.sel_slot > 0 ? WHITE : C_DIM);
    fy += 32;

    draw_input((Rectangle){fx, fy+18, 270, 32}, &ui.f_plate, 2, "Plate Number", m, 0); fy += 68;
    draw_input((Rectangle){fx, fy+18, 270, 32}, &ui.f_type,  3,
               "Vehicle Type  (Car / Truck / Motorcycle)", m, 0); fy += 68;

    int inf = infer_size(ui.f_type.b);
    if (inf >= 0) ui.sel_size = inf;

    DrawText("Vehicle Size", fx, fy, 12, C_SUB);  fy += 18;
    const char *sz[] = {"Small","Medium","Big"};
    Color sz_c[] = {C_GRN, C_BLU, C_RED};
    for (int s = 0; s < 3; s++) {
        Rectangle rb = {fx + s * 95, fy, 88, 30};
        if (draw_btn(rb, sz[s], ui.sel_size == s ? sz_c[s] : C_INP, m)) ui.sel_size = s;
        if (ui.sel_size == s) DrawRectangleLinesEx(rb, 2.0f, (Color){255,255,255,60});
    }
    fy += 44;

    draw_input((Rectangle){fx, fy+18, 100, 32}, &ui.f_entry, 4, "Entry Hour  (0-23)", m, 0);
    fy += 68;

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
            ui.sel_slot = -1; ui.active = 0;
        } else {
            msg_set(err, C_RED);
        }
    }
    fy += 52;
    DrawText(ui.msg, fx, fy, 13, ui.msg_col);
}

// ── SCREEN: REMOVE VEHICLE ───────────────────────────────────────────

static void draw_remove(Vector2 m) {
    int lw = 210, fx = CON_X + lw + 18, fy = CON_Y + 15;
    draw_slot_list(CON_X + 4, CON_Y + 4, lw, CON_H - 8, "Occupied Slots", 0, m);
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
    sprintf(line, "Plate : %s", v->plate);          DrawText(line, fx, fy, 14, WHITE); fy += 22;
    sprintf(line, "Type  : %s", v->type);           DrawText(line, fx, fy, 14, WHITE); fy += 22;
    sprintf(line, "Size  : %s", SIZE_NAMES[v->size]);DrawText(line, fx, fy, 14, WHITE); fy += 22;
    sprintf(line, "Entry : %02d:00", v->entry_hour); DrawText(line, fx, fy, 14, WHITE); fy += 30;

    draw_input((Rectangle){fx, fy+18, 110, 32}, &ui.f_exit, 5, "Exit Hour  (0-23)", m, 0);
    fy += 68;

    if (ui.f_exit.b[0]) {
        int exit_h = atoi(ui.f_exit.b);
        struct tm entry_tm = {0};
        entry_tm.tm_year  = v->entry_year - 1900;
        entry_tm.tm_mon   = v->entry_month - 1;
        entry_tm.tm_mday  = v->entry_day;
        entry_tm.tm_hour  = v->entry_hour;
        entry_tm.tm_isdst = -1;
        time_t entry_t = mktime(&entry_tm);
        time_t now_t   = time(NULL);
        struct tm *today = localtime(&now_t);
        struct tm exit_tm = *today;
        exit_tm.tm_hour = exit_h; exit_tm.tm_min = 0;
        exit_tm.tm_sec  = 0;     exit_tm.tm_isdst = -1;
        time_t exit_t = mktime(&exit_tm);
        int hours = (int)(difftime(exit_t, entry_t) / 3600.0);
        if (hours <= 0) hours = 1;
        double fee = calc_fee(v->size, hours);
        char preview[64];
        sprintf(preview, "Fee preview:  %d hr  x  $%.2f  =  $%.2f",
                hours, fee_rates[v->size], fee);
        DrawText(preview, fx, fy, 13, (Color){255, 200, 50, 255});
        fy += 28;
    }

    if (!ui.confirm_remove || ui.confirm_slot_id != ui.sel_slot) {
        if (draw_btn((Rectangle){fx, fy, 225, 42}, "REMOVE VEHICLE", C_RED, m)) {
            ui.confirm_remove  = 1;
            ui.confirm_slot_id = ui.sel_slot;
            msg_set("", WHITE);
        }
    } else {
        DrawText("Are you sure?", fx, fy - 2, 14, (Color){255, 200, 50, 255});
        if (draw_btn((Rectangle){fx + 120, fy - 4, 110, 36}, "CONFIRM", C_RED, m)) {
            char err[128] = ""; double fee; int hours;
            if (remove_car(ui.sel_slot, atoi(ui.f_exit.b), &fee, &hours, err)) {
                char ok[80];
                sprintf(ok, "Removed!  %d hr    Fee: $%.2f    Total: $%.2f",
                        hours, fee, income);
                msg_set(ok, (Color){60, 220, 100, 255});
                memset(&ui.f_exit, 0, sizeof(TF));
                ui.sel_slot = -1; ui.active = 0; ui.confirm_remove = 0;
            } else {
                msg_set(err, C_RED);
                ui.confirm_remove = 0;
            }
        }
        if (draw_btn((Rectangle){fx + 238, fy - 4, 90, 36}, "CANCEL", C_INP, m)) {
            ui.confirm_remove = 0;
            msg_set("", WHITE);
        }
    }
    fy += 52;
    DrawText(ui.msg, fx, fy, 13, ui.msg_col);
}

// ── SCREEN: UPDATE INFO ──────────────────────────────────────────────

static void draw_update(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 15;
    draw_heading(x, y, "Update Vehicle Info");  y += 40;

    draw_input((Rectangle){x, y+18, 150, 32}, &ui.f_uid, 8, "Slot ID", m, 0);
    if (draw_btn((Rectangle){x + 160, y+18, 90, 32}, "Load", C_BLU, m)) {
        int id = atoi(ui.f_uid.b), idx = find_slot(id);
        if (idx < 0)               msg_set("Slot not found", C_RED);
        else if (!lot[idx].occupied) msg_set("Slot is empty", C_RED);
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

    draw_input((Rectangle){x, y+18, 270, 32}, &ui.f_uplate, 9,  "Plate", m, 0); y += 68;
    draw_input((Rectangle){x, y+18, 270, 32}, &ui.f_utype,  10, "Type",  m, 0); y += 68;

    int upd_inf = infer_size(ui.f_utype.b);
    if (upd_inf >= 0) ui.upd_size = upd_inf;

    DrawText("Size", x, y, 12, C_SUB);  y += 18;
    const char *sz[] = {"Small","Medium","Big"};
    Color sz_c[] = {C_GRN, C_BLU, C_RED};
    for (int s = 0; s < 3; s++) {
        Rectangle rb = {x + s * 95, y, 88, 30};
        if (draw_btn(rb, sz[s], ui.upd_size == s ? sz_c[s] : C_INP, m)) ui.upd_size = s;
        if (ui.upd_size == s) DrawRectangleLinesEx(rb, 2.0f, (Color){255,255,255,60});
    }
    y += 48;

    if (draw_btn((Rectangle){x, y, 200, 42}, "SAVE CHANGES", C_GRN, m)) {
        char err[128] = "";
        if (update_car(ui.sel_slot, ui.f_uplate.b, ui.f_utype.b, ui.upd_size, err)) {
            msg_set("Updated successfully!", (Color){60, 220, 100, 255});
            ui.sel_slot = -1; ui.active = 0;
            memset(&ui.f_uid,    0, sizeof(TF));
            memset(&ui.f_uplate, 0, sizeof(TF));
            memset(&ui.f_utype,  0, sizeof(TF));
        } else {
            msg_set(err, C_RED);
        }
    }
    y += 52;
    DrawText(ui.msg, x, y, 13, ui.msg_col);
}

// ── SCREEN: SEARCH ───────────────────────────────────────────────────

static void draw_search(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 15;
    draw_heading(x, y, "Search");  y += 40;

    if (draw_btn((Rectangle){x,       y, 150, 34}, "By Plate",
                 ui.search_mode == 0 ? C_BLU : C_INP, m))
        { ui.search_mode = 0; ui.search_idx = -1; }
    if (draw_btn((Rectangle){x + 158, y, 150, 34}, "By Slot ID",
                 ui.search_mode == 1 ? C_BLU : C_INP, m))
        { ui.search_mode = 1; ui.search_idx = -1; }
    y += 50;

    draw_input((Rectangle){x, y+18, 300, 32}, &ui.f_search, 6,
               ui.search_mode == 0 ? "Plate Number" : "Slot ID", m, 0);
    y += 68;

    if (draw_btn((Rectangle){x, y, 130, 36}, "SEARCH", C_BLU, m)) {
        ui.search_idx = ui.search_mode == 0
            ? find_plate(ui.f_search.b)
            : find_slot(atoi(ui.f_search.b));
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

// ── SCREEN: SUMMARY ──────────────────────────────────────────────────

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
    sprintf(s, "%d", num_slots);    draw_card(x,       y, 195, 76, "TOTAL SLOTS",  s, C_CARD);
    sprintf(s, "%d", occ);          draw_card(x + 205, y, 195, 76, "OCCUPIED",     s, C_RED);
    sprintf(s, "%d", num_slots-occ);draw_card(x + 410, y, 195, 76, "AVAILABLE",    s, C_GRN);
    sprintf(s, "$%.2f", income);    draw_card(x + 615, y, 210, 76, "TOTAL INCOME", s, (Color){130, 85, 15, 255});
    y += 96;

    DrawText("Breakdown of occupied slots by size:", x, y, 13, C_SUB); y += 26;
    sprintf(s, "Small  : %d", sm);  DrawText(s, x, y, 14, WHITE); y += 22;
    sprintf(s, "Medium : %d", med); DrawText(s, x, y, 14, WHITE); y += 22;
    sprintf(s, "Big    : %d", big); DrawText(s, x, y, 14, WHITE); y += 32;

    DrawText("Fee rates per hour:", x, y, 13, C_SUB); y += 24;
    char r[48];
    sprintf(r, "Small  : $%.2f / hr", fee_rates[0]); DrawText(r, x, y, 13, C_DIM); y += 20;
    sprintf(r, "Medium : $%.2f / hr", fee_rates[1]); DrawText(r, x, y, 13, C_DIM); y += 20;
    sprintf(r, "Big    : $%.2f / hr", fee_rates[2]); DrawText(r, x, y, 13, C_DIM);
}

// ── SCREEN: ADD SLOTS ────────────────────────────────────────────────

static void draw_add_slots(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 20;
    draw_heading(x, y, "Add New Parking Slots");  y += 50;
    char cur[40]; sprintf(cur, "Current slot count:  %d", num_slots);
    DrawText(cur, x, y, 15, C_SUB);  y += 42;
    draw_input((Rectangle){x, y+18, 170, 36}, &ui.f_addslots, 7,
               "How many slots to add?", m, 0);  y += 72;
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

// ── SCREEN: SAVE / LOAD ──────────────────────────────────────────────

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

// ── STARTUP OVERLAY ───────────────────────────────────────────────────

static void draw_startup(Vector2 m) {
    DrawRectangle(0, 0, SCR_W, SCR_H, (Color){0, 0, 0, 190});
    int bh = ui.has_file ? 265 : 195;
    int bw = 500, bx = (SCR_W - bw) / 2, by = (SCR_H - bh) / 2;
    DrawRectangle(bx, by, bw, bh, C_PANEL);
    DrawRectangleLinesEx((Rectangle){bx, by, bw, bh}, 2.0f, C_BLU);
    DrawText("PARKING MANAGEMENT SYSTEM", bx + 20, by + 16, 18, WHITE);
    DrawRectangle(bx + 20, by + 42, bw - 40, 1, C_BOR);
    if (ui.has_file) {
        DrawText("Save file found:  parking_data.txt", bx+20, by+54, 13, C_SUB);
        if (draw_btn((Rectangle){bx+20, by+84, 220, 44}, "LOAD SAVED DATA", C_GRN, m)) {
            char err[128] = "";
            if (load_file(err)) { ui.started = 1; ui.scr = S_VIEW; }
            else                  msg_set(err, C_RED);
        }
        DrawText("\xe2\x80\x94 or start a new lot \xe2\x80\x94", bx + 150, by + 140, 12, C_DIM);
        draw_input((Rectangle){bx+20, by+164, 165, 34}, &ui.f_start, 1, NULL, m, 0);
        if (draw_btn((Rectangle){bx+194, by+164, 100, 34}, "CREATE", C_BLU, m)
            || IsKeyPressed(KEY_ENTER)) {
            int n = atoi(ui.f_start.b);
            if (n > 0) { init_lot(n); ui.started = 1; ui.scr = S_VIEW; }
            else msg_set("Enter a valid size", C_RED);
        }
    } else {
        DrawText("Enter the parking lot size to begin:", bx+20, by+54, 13, C_SUB);
        draw_input((Rectangle){bx+20, by+86, 200, 36}, &ui.f_start, 1, NULL, m, 0);
        if (draw_btn((Rectangle){bx+228, by+86, 120, 36}, "CREATE", C_BLU, m)
            || IsKeyPressed(KEY_ENTER)) {
            int n = atoi(ui.f_start.b);
            if (n > 0) { init_lot(n); ui.started = 1; ui.scr = S_VIEW; }
            else msg_set("Enter a positive number", C_RED);
        }
    }
    DrawText(ui.msg, bx + 20, by + bh - 24, 12, ui.msg_col);
}

// ── SCREEN: REPORTS ──────────────────────────────────────────────────

static void draw_reports(Vector2 m) {
    static struct Transaction results[MAX_REPORTS];
    static int result_count = -1;
    static int rep_scroll   = 0;

    int x = CON_X + 20, y = CON_Y + 15;
    draw_heading(x, y, "Reports");  y += 40;

    if (draw_btn((Rectangle){x, y, 120, 34},
                 "DAILY", ui.report_mode == 0 ? C_BLU : C_INP, m))
        { ui.report_mode = 0; result_count = -1; }
    if (draw_btn((Rectangle){x+128, y, 120, 34},
                 "MONTHLY", ui.report_mode == 1 ? C_BLU : C_INP, m))
        { ui.report_mode = 1; result_count = -1; }

    int dx = x + 270;
    if (ui.report_mode == 0) {
        draw_input((Rectangle){dx, y, 55, 34}, &ui.f_rep_day, 13, "Day", m, 0);
        dx += 65;
    }
    draw_input((Rectangle){dx, y, 60, 34}, &ui.f_rep_month, 14, "Month", m, 0); dx += 70;
    draw_input((Rectangle){dx, y, 75, 34}, &ui.f_rep_year,  15, "Year",  m, 0); dx += 85;

    if (draw_btn((Rectangle){dx, y, 100, 34}, "SEARCH", C_GRN, m)) {
        int yr = atoi(ui.f_rep_year.b), mo = atoi(ui.f_rep_month.b);
        int dy = (ui.report_mode == 0) ? atoi(ui.f_rep_day.b) : 0;
        if (yr > 0 && mo >= 1 && mo <= 12) {
            result_count = read_reports(yr, mo, dy, results, MAX_REPORTS);
            rep_scroll = 0; msg_set("", WHITE);
        } else {
            msg_set("Invalid date \xe2\x80\x94 enter a valid month (1-12) and year.", C_RED);
        }
    }
    dx += 108;
    if (draw_btn((Rectangle){dx, y, 120, 34}, "EXPORT CSV",
                 result_count > 0 ? C_BLU : C_INP, m) && result_count > 0) {
        FILE *ef = fopen("reports_export.csv", "w");
        if (ef) {
            fprintf(ef, "Date,Plate,Type,Size,Entry,Exit,Hours,Fee\n");
            for (int i = 0; i < result_count; i++)
                fprintf(ef, "%04d-%02d-%02d,%s,%s,%s,%02d:00,%02d:00,%d,%.2f\n",
                        results[i].year, results[i].month, results[i].day,
                        results[i].plate, results[i].type,
                        SIZE_NAMES[results[i].size],
                        results[i].entry_h, results[i].exit_h,
                        results[i].hours, results[i].fee);
            fclose(ef);
            msg_set("Exported to reports_export.csv", (Color){60, 220, 100, 255});
        } else {
            msg_set("Could not write reports_export.csv", C_RED);
        }
    }
    y += 50;
    DrawText(ui.msg, x, y, 12, ui.msg_col);  y += 20;

    if (result_count < 0) {
        DrawText("Select a date and click SEARCH.", x, y + 10, 13, C_DIM);
        return;
    }

    double total_fee = 0;
    for (int i = 0; i < result_count; i++) total_fee += results[i].fee;
    double avg_fee = result_count > 0 ? total_fee / result_count : 0.0;

    char sv[32];
    sprintf(sv, "%d", result_count); draw_card(x,       y, 185, 68, "CUSTOMERS", sv, C_CARD);
    sprintf(sv, "$%.2f", total_fee); draw_card(x + 195, y, 185, 68, "TOTAL FEE", sv, (Color){130, 85, 15, 255});
    sprintf(sv, "$%.2f", avg_fee);   draw_card(x + 390, y, 185, 68, "AVG FEE",   sv, C_CARD);
    y += 84;

    if (result_count == 0) {
        DrawText("No records for this period.", x, y + 10, 13, C_DIM);
        return;
    }

    int tw = CON_W - 40;
    DrawRectangle(x, y, tw, 26, C_HEADER);
    DrawText("Date",  x+8,   y+6, 12, C_SUB); DrawText("Plate", x+115, y+6, 12, C_SUB);
    DrawText("Type",  x+235, y+6, 12, C_SUB); DrawText("Size",  x+355, y+6, 12, C_SUB);
    DrawText("Hours", x+435, y+6, 12, C_SUB); DrawText("Fee",   x+515, y+6, 12, C_SUB);
    y += 26;

    int row_h = 26, clip_h = SCR_H - y - 22, visible = clip_h / row_h;
    scroll_clamp(&rep_scroll, result_count, visible, (Rectangle){x, y, tw, clip_h}, m);

    BeginScissorMode(x, y, tw, clip_h);
    for (int i = rep_scroll; i < result_count; i++) {
        int ry = y + (i - rep_scroll) * row_h;
        if (ry > SCR_H) break;
        Color bg = (i % 2 == 0) ? C_PANEL : C_BG;
        bg.r += 4; bg.g += 4; bg.b += 4;
        DrawRectangle(x, ry, tw, row_h, bg);
        DrawRectangle(x, ry + row_h - 1, tw, 1, (Color){20, 40, 90, 100});
        char tmp[32];
        sprintf(tmp, "%04d-%02d-%02d", results[i].year, results[i].month, results[i].day);
        DrawText(tmp,                         x+8,   ry+6, 12, C_SUB);
        DrawText(results[i].plate,            x+115, ry+6, 12, WHITE);
        DrawText(results[i].type,             x+235, ry+6, 12, C_SUB);
        DrawText(SIZE_NAMES[results[i].size], x+355, ry+6, 12, C_SUB);
        sprintf(tmp, "%d hr", results[i].hours); DrawText(tmp, x+435, ry+6, 12, C_SUB);
        sprintf(tmp, "$%.2f", results[i].fee);   DrawText(tmp, x+515, ry+6, 12, (Color){60,220,100,255});
    }
    EndScissorMode();
    char footer[48]; sprintf(footer, "%d records", result_count);
    DrawText(footer, x, SCR_H - 16, 11, C_DIM);
}

// ── SCREEN: SETTINGS ─────────────────────────────────────────────────

static void save_admin_cfg(void) {
    FILE *cf = fopen("admin.cfg", "w");
    if (!cf) return;
    fprintf(cf, "%s\n%s\n%.2f %.2f %.2f\n",
            ui.admin_user, ui.admin_pass,
            fee_rates[0], fee_rates[1], fee_rates[2]);
    fclose(cf);
}

static void draw_settings(Vector2 m) {
    int x = CON_X + 20, y = CON_Y + 15;
    draw_heading(x, y, "Settings");  y += 44;

    DrawText("Change Credentials", x, y, 13, C_SUB);
    DrawRectangle(x, y + 18, 260, 1, C_BOR);  y += 28;
    draw_input((Rectangle){x, y+18, 260, 32}, &ui.f_set_user, 16, "Username",     m, 0); y += 68;
    draw_input((Rectangle){x, y+18, 260, 32}, &ui.f_set_pass, 17, "New Password", m, 1); y += 68;

    if (draw_btn((Rectangle){x, y, 200, 38}, "SAVE CREDENTIALS", C_GRN, m)) {
        if (!ui.f_set_user.b[0]) {
            msg_set("Username cannot be empty", C_RED);
        } else {
            strncpy(ui.admin_user, ui.f_set_user.b, 63);
            if (ui.f_set_pass.b[0]) strncpy(ui.admin_pass, ui.f_set_pass.b, 63);
            save_admin_cfg();
            memset(&ui.f_set_pass, 0, sizeof(TF));
            msg_set("Credentials saved.", (Color){60, 220, 100, 255});
        }
    }
    y += 56;

    DrawText("Fee Rates  ($ per hour)", x, y, 13, C_SUB);
    DrawRectangle(x, y + 18, 260, 1, C_BOR);  y += 28;
    draw_input((Rectangle){x,      y+18, 75, 32}, &ui.f_set_r0, 18, "Small",  m, 0);
    draw_input((Rectangle){x+85,   y+18, 75, 32}, &ui.f_set_r1, 19, "Medium", m, 0);
    draw_input((Rectangle){x+170,  y+18, 75, 32}, &ui.f_set_r2, 20, "Big",    m, 0);
    y += 68;

    if (draw_btn((Rectangle){x, y, 160, 38}, "SAVE RATES", C_GRN, m)) {
        double r0 = atof(ui.f_set_r0.b), r1 = atof(ui.f_set_r1.b), r2 = atof(ui.f_set_r2.b);
        if (r0 <= 0 || r1 <= 0 || r2 <= 0) {
            msg_set("All rates must be greater than 0", C_RED);
        } else {
            fee_rates[0] = r0; fee_rates[1] = r1; fee_rates[2] = r2;
            snprintf(ui.f_set_r0.b, FLEN, "%.2f", r0);
            snprintf(ui.f_set_r1.b, FLEN, "%.2f", r1);
            snprintf(ui.f_set_r2.b, FLEN, "%.2f", r2);
            save_admin_cfg();
            msg_set("Rates saved.", (Color){60, 220, 100, 255});
        }
    }
    y += 52;
    DrawText(ui.msg, x, y, 13, ui.msg_col);
}

// ── SCREEN: ROLE SELECTION / ADMIN LOGIN ─────────────────────────────

static void draw_role_select(Vector2 m) {
    DrawRectangle(0, 0, SCR_W, SCR_H, (Color){0, 0, 0, 190});
    int bw = 500, bh = 180, bx = (SCR_W-bw)/2, by = (SCR_H-bh)/2;
    DrawRectangle(bx, by, bw, bh, C_PANEL);
    DrawRectangleLinesEx((Rectangle){bx, by, bw, bh}, 2.0f, C_BLU);
    DrawText("PARKING MANAGEMENT SYSTEM", bx+20, by+16, 18, WHITE);
    DrawRectangle(bx+20, by+42, bw-40, 1, C_BOR);
    DrawText("Please select your role to continue:", bx+20, by+56, 13, C_SUB);
    if (draw_btn((Rectangle){bx+20, by+90, 215, 46}, "ADMINISTRATOR", C_BLU, m)) {
        ui.scr = S_ADMIN_LOGIN; ui.active = 11;
        memset(&ui.f_username, 0, sizeof(TF));
        memset(&ui.f_password, 0, sizeof(TF));
        msg_set("", WHITE);
    }
    if (draw_btn((Rectangle){bx+265, by+90, 215, 46}, "USER", C_GRN, m)) {
        ui.role = 1; ui.scr = S_STARTUP; ui.active = 1;
        msg_set("", WHITE);
    }
    DrawText(ui.msg, bx+20, by+bh-24, 12, ui.msg_col);
}

static void draw_admin_login(Vector2 m) {
    DrawRectangle(0, 0, SCR_W, SCR_H, (Color){0, 0, 0, 190});
    int bw = 500, bh = 250, bx = (SCR_W-bw)/2, by = (SCR_H-bh)/2;
    DrawRectangle(bx, by, bw, bh, C_PANEL);
    DrawRectangleLinesEx((Rectangle){bx, by, bw, bh}, 2.0f, C_BLU);
    DrawText("PARKING MANAGEMENT SYSTEM", bx+20, by+16, 18, WHITE);
    DrawRectangle(bx+20, by+42, bw-40, 1, C_BOR);
    draw_heading(bx+20, by+54, "Administrator Login");
    draw_input((Rectangle){bx+20, by+100, 460, 32}, &ui.f_username, 11, "Username", m, 0);
    draw_input((Rectangle){bx+20, by+158, 460, 32}, &ui.f_password, 12, "Password", m, 1);
    if (draw_btn((Rectangle){bx+20, by+204, 205, 38}, "LOGIN", C_GRN, m)
        || IsKeyPressed(KEY_ENTER)) {
        if (strcmp(ui.f_username.b, ui.admin_user) == 0 &&
            strcmp(ui.f_password.b, ui.admin_pass) == 0) {
            ui.role = 2; ui.scr = S_STARTUP; ui.active = 1;
            msg_set("", WHITE);
        } else {
            msg_set("Invalid username or password.", C_RED);
        }
    }
    if (draw_btn((Rectangle){bx+275, by+204, 205, 38}, "BACK", C_INP, m)) {
        ui.scr = S_ROLE_SELECT; ui.active = 0;
        msg_set("", WHITE);
    }
    DrawText(ui.msg, bx+20, by+bh-24, 12, ui.msg_col);
}

// ── SCREEN: VEHICLE HISTORY ──────────────────────────────────────────

static void draw_history(Vector2 m) {
    static struct Transaction results[MAX_REPORTS];
    static int result_count = -1;
    static int hist_scroll  = 0;

    int x = CON_X + 20, y = CON_Y + 15;
    draw_heading(x, y, "Vehicle History");  y += 40;

    draw_input((Rectangle){x, y, 220, 34}, &ui.f_hist_plate, 21, "Plate Number", m, 0);
    if (draw_btn((Rectangle){x+228, y, 100, 34}, "SEARCH", C_GRN, m)) {
        if (!ui.f_hist_plate.b[0]) {
            msg_set("Enter a plate number to search.", C_YEL);
            result_count = -1;
        } else {
            result_count = read_reports_by_plate(ui.f_hist_plate.b, results, MAX_REPORTS);
            hist_scroll = 0; msg_set("", WHITE);
        }
    }
    if (draw_btn((Rectangle){x+336, y, 80, 34}, "CLEAR", C_INP, m)) {
        ui.f_hist_plate.b[0] = '\0';
        result_count = -1; hist_scroll = 0;
        msg_set("", WHITE);
    }
    y += 50;
    DrawText(ui.msg, x, y, 12, ui.msg_col);  y += 20;

    if (result_count < 0) {
        DrawText("Search a plate to see its parking history.", x, y + 10, 13, C_DIM);
        return;
    }

    double total_fee = 0; int total_hours = 0;
    for (int i = 0; i < result_count; i++) {
        total_fee   += results[i].fee;
        total_hours += results[i].hours;
    }

    char sv[32];
    sprintf(sv, "%d", result_count);    draw_card(x,       y, 185, 68, "TOTAL VISITS", sv, C_CARD);
    sprintf(sv, "$%.2f", total_fee);    draw_card(x + 195, y, 185, 68, "TOTAL PAID",   sv, (Color){130, 85, 15, 255});
    sprintf(sv, "%d hr", total_hours);  draw_card(x + 390, y, 185, 68, "TOTAL HOURS",  sv, C_CARD);
    y += 84;

    if (result_count == 0) {
        DrawText("No records found for this plate.", x, y + 10, 13, C_DIM);
        return;
    }

    int tw = CON_W - 40;
    DrawRectangle(x, y, tw, 26, C_HEADER);
    DrawText("Date",  x+8,   y+6, 12, C_SUB); DrawText("Type",  x+115, y+6, 12, C_SUB);
    DrawText("Size",  x+235, y+6, 12, C_SUB); DrawText("Entry", x+330, y+6, 12, C_SUB);
    DrawText("Exit",  x+410, y+6, 12, C_SUB); DrawText("Hours", x+490, y+6, 12, C_SUB);
    DrawText("Fee",   x+570, y+6, 12, C_SUB);
    y += 26;

    int row_h = 26, clip_h = SCR_H - y - 22, visible = clip_h / row_h;
    scroll_clamp(&hist_scroll, result_count, visible, (Rectangle){x, y, tw, clip_h}, m);

    BeginScissorMode(x, y, tw, clip_h);
    for (int i = hist_scroll; i < result_count; i++) {
        int ry = y + (i - hist_scroll) * row_h;
        if (ry > SCR_H) break;
        Color bg = (i % 2 == 0) ? C_PANEL : C_BG;
        bg.r += 4; bg.g += 4; bg.b += 4;
        DrawRectangle(x, ry, tw, row_h, bg);
        DrawRectangle(x, ry + row_h - 1, tw, 1, (Color){20, 40, 90, 100});
        char tmp[32];
        sprintf(tmp, "%04d-%02d-%02d", results[i].year, results[i].month, results[i].day);
        DrawText(tmp,                         x+8,   ry+6, 12, C_SUB);
        DrawText(results[i].type,             x+115, ry+6, 12, C_SUB);
        DrawText(SIZE_NAMES[results[i].size], x+235, ry+6, 12, C_SUB);
        sprintf(tmp, "%02d:00", results[i].entry_h); DrawText(tmp, x+330, ry+6, 12, C_SUB);
        sprintf(tmp, "%02d:00", results[i].exit_h);  DrawText(tmp, x+410, ry+6, 12, C_SUB);
        sprintf(tmp, "%d hr",   results[i].hours);   DrawText(tmp, x+490, ry+6, 12, C_SUB);
        sprintf(tmp, "$%.2f",   results[i].fee);     DrawText(tmp, x+570, ry+6, 12, (Color){60,220,100,255});
    }
    EndScissorMode();
    char footer[48]; sprintf(footer, "%d records", result_count);
    DrawText(footer, x, SCR_H - 16, 11, C_DIM);
}

// ── PUBLIC API ────────────────────────────────────────────────────────

void ui_init(void) {
    memset(&ui, 0, sizeof(ui));
    ui.scr        = S_ROLE_SELECT;
    ui.sel_slot   = -1;
    ui.search_idx = -1;
    ui.msg_col    = WHITE;

    time_t now_t = time(NULL);
    struct tm *now_tm = localtime(&now_t);
    snprintf(ui.f_rep_day.b,   FLEN, "%d", now_tm->tm_mday);
    snprintf(ui.f_rep_month.b, FLEN, "%d", now_tm->tm_mon + 1);
    snprintf(ui.f_rep_year.b,  FLEN, "%d", now_tm->tm_year + 1900);

    FILE *cf = fopen("admin.cfg", "r");
    if (cf) {
        fscanf(cf, "%63s %63s", ui.admin_user, ui.admin_pass);
        double r0, r1, r2;
        if (fscanf(cf, "%lf %lf %lf", &r0, &r1, &r2) == 3) {
            fee_rates[0] = r0; fee_rates[1] = r1; fee_rates[2] = r2;
        }
        fclose(cf);
    } else {
        strncpy(ui.admin_user, "admin", 63);
        strncpy(ui.admin_pass, "1234",  63);
        cf = fopen("admin.cfg", "w");
        if (cf) { fprintf(cf, "admin\n1234\n1.00 2.00 3.00\n"); fclose(cf); }
    }

    snprintf(ui.f_set_user.b, FLEN, "%s",   ui.admin_user);
    snprintf(ui.f_set_r0.b,   FLEN, "%.2f", fee_rates[0]);
    snprintf(ui.f_set_r1.b,   FLEN, "%.2f", fee_rates[1]);
    snprintf(ui.f_set_r2.b,   FLEN, "%.2f", fee_rates[2]);

    FILE *f = fopen(DATA_FILE, "r");
    if (f) { fclose(f); ui.has_file = 1; }
}

void ui_update(void) { handle_keys(); }

void ui_draw(void) {
    Vector2 m = GetMousePosition();
    if (ui.scr == S_ROLE_SELECT) { draw_title_bar(); draw_role_select(m); return; }
    if (ui.scr == S_ADMIN_LOGIN) { draw_title_bar(); draw_admin_login(m); return; }
    if (!ui.started)             { draw_title_bar(); draw_startup(m);     return; }

    draw_title_bar();
    draw_stats_bar();
    draw_nav(m);
    DrawRectangle(CON_X, CON_Y, CON_W, CON_H, C_BG);

    switch (ui.scr) {
        case S_VIEW:     draw_view_all(m);  break;
        case S_PARK:     draw_park(m);      break;
        case S_REMOVE:   draw_remove(m);    break;
        case S_UPDATE:   draw_update(m);    break;
        case S_SEARCH:   draw_search(m);    break;
        case S_SUMMARY:  draw_summary(m);   break;
        case S_REPORTS:  draw_reports(m);   break;
        case S_SETTINGS: draw_settings(m);  break;
        case S_ADD:      draw_add_slots(m); break;
        case S_FILE:     draw_file(m);      break;
        case S_HISTORY:  draw_history(m);   break;
        default: break;
    }
}
