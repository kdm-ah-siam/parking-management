// main.c — initialise raylib window and run the main loop

#include <stdlib.h>
#include "raylib.h"
#include "parking.h"
#include "ui.h"

#define SCR_W 1200
#define SCR_H  740

int main(void) {
    InitWindow(SCR_W, SCR_H, "Parking Management System");
    SetTargetFPS(60);

    ui_init();

    while (!WindowShouldClose()) {
        ui_update();
        BeginDrawing();
        ClearBackground((Color){10, 17, 40, 255});
        ui_draw();
        EndDrawing();
    }

    if (num_slots > 0) { char serr[128] = ""; save_file(serr); }
    CloseWindow();
    free(lot);
    return 0;
}
