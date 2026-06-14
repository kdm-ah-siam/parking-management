// Approach B: Dashboard Parking Lot Manager

#include "raylib.h"
#include <string.h>
#include <stdio.h>

#define MAX_SPOTS 20
#define SCREEN_W  1100
#define SCREEN_H  720

typedef struct {
    char plate[12];
    int  type;       // 0=Car  1=Bike  2=Truck
    int  occupied;
} Spot;

static const char  *TYPE_NAMES[]  = { "Car", "Bike", "Truck" };
static const Color  TYPE_COLORS[] = {
    {160, 35, 35, 255},
    {30,  90,160, 255},
    {100, 60, 20, 255}
};

Spot spots[MAX_SPOTS];

/* active input: 0=none  1=park_plate  2=remove_plate  3=search_plate */
int  active_input  = 0;
char park_plate[12]   = {0};
int  park_type     = 0;
char remove_plate[12] = {0};
char search_plate[12] = {0};

/* feedback messages */
char msg_park[64]   = {0};
char msg_remove[64] = {0};
Color msg_park_col   = {0};
Color msg_remove_col = {0};

/* ---- helpers ---- */

void text_input(char *buf, int max_len) {
    int key = GetCharPressed();
    while (key > 0) {
        int len = (int)strlen(buf);
        if (key >= 32 && key <= 125 && len < max_len - 1) {
            buf[len] = (char)key;
            buf[len + 1] = '\0';
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(buf);
        if (len > 0) buf[len - 1] = '\0';
    }
}

int count_occupied(void) {
    int n = 0;
    for (int i = 0; i < MAX_SPOTS; i++) if (spots[i].occupied) n++;
    return n;
}

int find_plate(const char *p) {
    if (!p[0]) return -1;
    for (int i = 0; i < MAX_SPOTS; i++)
        if (spots[i].occupied && strcmp(spots[i].plate, p) == 0) return i;
    return -1;
}

int first_free(void) {
    for (int i = 0; i < MAX_SPOTS; i++) if (!spots[i].occupied) return i;
    return -1;
}

void draw_input_box(int x, int y, int w, int h, const char *text, int is_active) {
    Color bg  = is_active ? (Color){44, 58, 95, 255}  : (Color){34, 48, 78, 255};
    Color brd = is_active ? (Color){100,140,228,255}   : (Color){65, 82,118,255};
    DrawRectangle(x, y, w, h, bg);
    DrawRectangleLinesEx((Rectangle){x, y, w, h}, 1.5f, brd);
    DrawText(text, x + 6, y + (h - 14) / 2, 14, WHITE);
    if (is_active && (int)(GetTime() * 2) % 2)
        DrawRectangle(x + 6 + MeasureText(text, 14), y + (h - 14) / 2, 1, 14, WHITE);
}

/* ---- main ---- */

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Parking Lot - Approach B: Dashboard");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mouse   = GetMousePosition();
        int     clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        /* === UPDATE === */

        /* route keyboard to active field */
        if (active_input == 1) text_input(park_plate,   12);
        if (active_input == 2) text_input(remove_plate, 12);
        if (active_input == 3) text_input(search_plate, 12);

        if (clicked) {
            /* --- input field focus --- */
            Rectangle pp_box = {18, 532, 200, 28};
            Rectangle rp_box = {388, 532, 200, 28};
            Rectangle sp_box = {758, 532, 200, 28};

            if (CheckCollisionPointRec(mouse, pp_box))      active_input = 1;
            else if (CheckCollisionPointRec(mouse, rp_box)) active_input = 2;
            else if (CheckCollisionPointRec(mouse, sp_box)) active_input = 3;
            else active_input = 0;

            /* --- type selector in PARK panel --- */
            int tx[] = {18, 80, 145};
            int tw[] = {55, 58,  72};
            for (int t = 0; t < 3; t++) {
                Rectangle tb = {tx[t], 568, tw[t], 24};
                if (CheckCollisionPointRec(mouse, tb)) park_type = t;
            }

            /* --- PARK button --- */
            Rectangle park_btn = {18, 602, 100, 34};
            if (CheckCollisionPointRec(mouse, park_btn)) {
                if (!park_plate[0]) {
                    strcpy(msg_park, "Enter a plate number!");
                    msg_park_col = (Color){228, 80, 80, 255};
                } else {
                    int slot = first_free();
                    if (slot < 0) {
                        strcpy(msg_park, "Lot is full!");
                        msg_park_col = (Color){228, 80, 80, 255};
                    } else if (find_plate(park_plate) >= 0) {
                        strcpy(msg_park, "Plate already parked!");
                        msg_park_col = (Color){228, 160, 30, 255};
                    } else {
                        spots[slot].occupied = 1;
                        strcpy(spots[slot].plate, park_plate);
                        spots[slot].type = park_type;
                        snprintf(msg_park, sizeof(msg_park),
                            "Parked at P%02d!", slot + 1);
                        msg_park_col = (Color){80, 220, 110, 255};
                        park_plate[0] = '\0';
                    }
                }
            }

            /* --- REMOVE button --- */
            Rectangle remove_btn = {388, 580, 110, 34};
            if (CheckCollisionPointRec(mouse, remove_btn)) {
                int idx = find_plate(remove_plate);
                if (!remove_plate[0]) {
                    strcpy(msg_remove, "Enter a plate number!");
                    msg_remove_col = (Color){228, 80, 80, 255};
                } else if (idx < 0) {
                    strcpy(msg_remove, "Plate not found!");
                    msg_remove_col = (Color){228, 80, 80, 255};
                } else {
                    char removed_id[16];
                    snprintf(removed_id, sizeof(removed_id), "P%02d removed!", idx + 1);
                    strcpy(msg_remove, removed_id);
                    msg_remove_col = (Color){80, 220, 110, 255};
                    spots[idx].occupied = 0;
                    spots[idx].plate[0] = '\0';
                    remove_plate[0] = '\0';
                }
            }

            if (IsKeyPressed(KEY_ESCAPE)) active_input = 0;
        }

        /* === DRAW === */
        BeginDrawing();
        ClearBackground((Color){18, 26, 48, 255});

        /* title bar */
        DrawRectangle(0, 0, SCREEN_W, 58, (Color){22, 42, 88, 255});
        DrawText("PARKING LOT MANAGER", 18, 8, 28, WHITE);
        DrawText("Approach B  |  Dashboard View  |  PARK / REMOVE / SEARCH in the panel below",
            20, 42, 12, (Color){150, 170, 220, 255});

        /* stats cards (y=62) */
        int occ = count_occupied();
        int avail = MAX_SPOTS - occ;
        int pct = (occ * 100) / MAX_SPOTS;

        int card_xs[] = {10, 285, 560, 835};
        const char *card_labels[] = { "TOTAL SPOTS", "OCCUPIED", "AVAILABLE", "% FULL" };
        char card_vals[4][8];
        snprintf(card_vals[0], 8, "%d",  MAX_SPOTS);
        snprintf(card_vals[1], 8, "%d",  occ);
        snprintf(card_vals[2], 8, "%d",  avail);
        snprintf(card_vals[3], 8, "%d%%", pct);
        Color card_colors[] = {
            {38,  58, 110, 255},
            {140, 35,  35, 255},
            {35, 120,  55, 255},
            {110, 75,  20, 255}
        };

        for (int c = 0; c < 4; c++) {
            DrawRectangle(card_xs[c], 62, 262, 72, card_colors[c]);
            DrawRectangleLinesEx((Rectangle){card_xs[c], 62, 262, 72},
                1.5f, (Color){255,255,255,30});
            DrawText(card_labels[c], card_xs[c] + 12, 70, 13, (Color){200, 210, 235, 255});
            DrawText(card_vals[c],   card_xs[c] + 12, 96, 28, WHITE);
        }

        /* table header */
        int TABLE_Y = 142;
        DrawRectangle(0, TABLE_Y, SCREEN_W, 28, (Color){30, 45, 80, 255});
        /* column headers for both sides */
        int hx[] = {10, 65, 225, 315};
        const char *hdr[] = {"SPOT", "PLATE", "TYPE", "STATUS"};
        for (int h = 0; h < 4; h++) {
            DrawText(hdr[h], hx[h],            TABLE_Y + 7, 13, (Color){170, 185, 218, 255});
            DrawText(hdr[h], hx[h] + 550, TABLE_Y + 7, 13, (Color){170, 185, 218, 255});
        }
        DrawRectangle(550, TABLE_Y, 2, SCREEN_H - TABLE_Y, (Color){40, 58, 100, 255});

        /* table rows */
        int search_match = find_plate(search_plate);

        for (int i = 0; i < MAX_SPOTS; i++) {
            int row  = i % 10;
            int col  = i / 10;   /* 0 = left, 1 = right */
            int rx   = col * 550;
            int ry   = TABLE_Y + 28 + row * 30;

            Color row_bg = (i == search_match)
                ? (Color){100, 80, 20, 255}
                : (row % 2 == 0)
                    ? (Color){24, 34, 60, 255}
                    : (Color){28, 40, 68, 255};
            DrawRectangle(rx, ry, 550, 30, row_bg);

            char spot_id[6];
            snprintf(spot_id, sizeof(spot_id), "P%02d", i + 1);
            DrawText(spot_id, rx + 10, ry + 8, 14, (Color){180, 195, 228, 255});

            if (spots[i].occupied) {
                DrawText(spots[i].plate, rx + 65, ry + 8, 14, WHITE);
                DrawText(TYPE_NAMES[spots[i].type],
                    rx + 225, ry + 8, 13, (Color){188, 210, 255, 255});
                DrawRectangle(rx + 315, ry + 6, 90, 18,
                    (Color){140, 35, 35, 200});
                DrawText("OCCUPIED", rx + 318, ry + 9, 12, WHITE);
            } else {
                DrawText("---",  rx + 65,  ry + 8, 14, (Color){80, 90, 118, 255});
                DrawText("---",  rx + 225, ry + 8, 13, (Color){80, 90, 118, 255});
                DrawRectangle(rx + 315, ry + 6, 80, 18,
                    (Color){35, 120, 55, 200});
                DrawText("FREE", rx + 326, ry + 9, 12, WHITE);
            }

            /* row separator */
            DrawRectangle(rx, ry + 29, 550, 1, (Color){38, 52, 88, 255});
        }

        /* operations panel background */
        int PANEL_Y = TABLE_Y + 28 + 10 * 30;   /* = 448 */
        DrawRectangle(0, PANEL_Y, SCREEN_W, SCREEN_H - PANEL_Y, (Color){20, 30, 55, 255});
        DrawRectangle(0, PANEL_Y, SCREEN_W, 2, (Color){58, 88, 148, 255});

        /* column dividers */
        DrawRectangle(366, PANEL_Y, 2, SCREEN_H - PANEL_Y, (Color){38, 55, 95, 255});
        DrawRectangle(734, PANEL_Y, 2, SCREEN_H - PANEL_Y, (Color){38, 55, 95, 255});

        int py = PANEL_Y + 10;

        /* --- PARK panel (x=0) --- */
        DrawText("PARK VEHICLE", 18, py, 15, (Color){80, 220, 110, 255});

        DrawText("Plate:", 18, py + 54, 13, (Color){170, 185, 218, 255});
        draw_input_box(18, py + 72, 200, 28, park_plate, active_input == 1);

        DrawText("Type:", 18, py + 108, 13, (Color){170, 185, 218, 255});
        int tx2[] = {18, 80, 145};
        int tw2[] = {55, 58,  72};
        for (int t = 0; t < 3; t++) {
            Rectangle tb = {tx2[t], py + 126, tw2[t], 24};
            Color bg_t = (park_type == t)
                ? (Color){38, 78, 198, 255} : (Color){45, 60, 98, 255};
            Color fg_t = WHITE;
            DrawRectangleRec(tb, bg_t);
            DrawRectangleLinesEx(tb, 1.0f, (Color){75, 95, 175, 255});
            DrawText(TYPE_NAMES[t],
                tx2[t] + (tw2[t] - MeasureText(TYPE_NAMES[t], 12)) / 2,
                py + 130, 12, fg_t);
        }

        Rectangle park_btn2 = {18, py + 158, 110, 34};
        int ph2 = CheckCollisionPointRec(mouse, park_btn2);
        DrawRectangleRec(park_btn2,
            ph2 ? (Color){28, 145, 58, 255} : (Color){38, 165, 68, 255});
        DrawText("PARK VEHICLE",
            18 + (110 - MeasureText("PARK VEHICLE", 12)) / 2,
            py + 169, 12, WHITE);

        DrawText(msg_park, 18, py + 200, 12, msg_park_col);

        /* --- REMOVE panel (x=368) --- */
        DrawText("REMOVE VEHICLE", 388, py, 15, (Color){228, 80, 80, 255});

        DrawText("Plate:", 388, py + 54, 13, (Color){170, 185, 218, 255});
        draw_input_box(388, py + 72, 200, 28, remove_plate, active_input == 2);

        Rectangle remove_btn2 = {388, py + 116, 120, 34};
        int rh2 = CheckCollisionPointRec(mouse, remove_btn2);
        DrawRectangleRec(remove_btn2,
            rh2 ? (Color){145, 28, 28, 255} : (Color){165, 42, 42, 255});
        DrawText("REMOVE",
            388 + (120 - MeasureText("REMOVE", 14)) / 2,
            py + 125, 14, WHITE);

        DrawText(msg_remove, 388, py + 162, 12, msg_remove_col);

        DrawText("Removes by license plate.",  388, py + 202, 12, (Color){100, 115, 148, 255});
        DrawText("Spot is freed immediately.", 388, py + 218, 12, (Color){100, 115, 148, 255});

        /* --- SEARCH panel (x=736) --- */
        DrawText("SEARCH PLATE", 758, py, 15, (Color){200, 150, 20, 255});

        DrawText("Enter plate:", 758, py + 54, 13, (Color){170, 185, 218, 255});
        draw_input_box(758, py + 72, 200, 28, search_plate, active_input == 3);

        DrawText("Result:", 758, py + 112, 13, (Color){170, 185, 218, 255});

        if (search_plate[0]) {
            if (search_match >= 0) {
                char res[48];
                snprintf(res, sizeof(res), "Found at Spot P%02d", search_match + 1);
                DrawText(res, 758, py + 132, 14, (Color){80, 220, 110, 255});
                snprintf(res, sizeof(res), "Type: %s", TYPE_NAMES[spots[search_match].type]);
                DrawText(res, 758, py + 152, 13, (Color){170, 185, 218, 255});
                DrawText("(highlighted in table)", 758, py + 172, 12,
                    (Color){200, 150, 20, 255});
            } else {
                DrawText("Plate not in the lot.", 758, py + 132, 14,
                    (Color){228, 80, 80, 255});
            }
        } else {
            DrawText("Type a plate above.", 758, py + 132, 13,
                (Color){80, 95, 130, 255});
        }

        /* Click hint */
        DrawText("Click a field to type. Press ESC to deselect.",
            18, SCREEN_H - 18, 12, (Color){68, 82, 115, 255});

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
