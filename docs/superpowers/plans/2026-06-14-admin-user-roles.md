# Admin / User Role System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a role-selection login screen at startup so Administrators authenticate with username/password (stored in `admin.cfg`) and Users go straight to the parking program with a restricted nav (no Add Slots or Save/Load). Both roles get a Logout button that returns to the role-selection screen.

**Architecture:** All changes live in `ui.c`. Two new `Screen` states (`S_ROLE_SELECT`, `S_ADMIN_LOGIN`) are added to the existing enum and handled as modal overlays before `ui.started` becomes true. A `role` field on the `ui` struct drives conditional nav rendering. `admin.cfg` is loaded once at startup.

**Tech Stack:** C11, Raylib (GUI), gcc, make

---

## File Map

| File | Action | What changes |
|---|---|---|
| `ui.c` | Modify | All new logic — enum, struct, new screens, credential loading, nav filtering, logout |
| `admin.cfg` | Create (auto) | Two-line credentials file, created by the program if missing |

No other files change.

---

### Task 1: Extend Screen enum, ui struct, and tf()

**Files:**
- Modify: `ui.c:41-44` (Screen enum)
- Modify: `ui.c:51-78` (ui struct)
- Modify: `ui.c:81-95` (tf function)

- [ ] **Step 1: Add two new states to the Screen enum**

Find this block in `ui.c` (lines 41-44):
```c
typedef enum {
    S_STARTUP, S_VIEW, S_PARK, S_REMOVE,
    S_UPDATE,  S_SEARCH, S_SUMMARY, S_ADD, S_FILE
} Screen;
```
Replace with:
```c
typedef enum {
    S_STARTUP, S_VIEW, S_PARK, S_REMOVE,
    S_UPDATE,  S_SEARCH, S_SUMMARY, S_ADD, S_FILE,
    S_ROLE_SELECT, S_ADMIN_LOGIN
} Screen;
```

- [ ] **Step 2: Add role and credential fields to the ui struct**

Find the closing brace of the `ui` struct (after `lscroll`). The last lines of the struct look like:
```c
    char  msg[128];
    Color msg_col;
    int   scroll;       // view-all table scroll
    int   lscroll;      // slot-list panel scroll
} ui;
```
Replace with:
```c
    char  msg[128];
    Color msg_col;
    int   scroll;       // view-all table scroll
    int   lscroll;      // slot-list panel scroll

    int  role;            // 0=none  1=user  2=admin
    TF   f_username;      // id 11
    TF   f_password;      // id 12  (masked display)
    char admin_user[64];
    char admin_pass[64];
} ui;
```

- [ ] **Step 3: Add cases 11 and 12 to tf()**

Find `tf()` (lines 81-95). The `default` case is the last entry:
```c
        default: return NULL;
    }
```
Add the two new cases directly before `default`:
```c
        case 11: return &ui.f_username;
        case 12: return &ui.f_password;
        default: return NULL;
```

- [ ] **Step 4: Build to verify no compile errors**

```bash
cd /Users/macbookpro/parking_management && make clean && make
```
Expected: compiles with no errors. Program behaviour unchanged (still starts at startup overlay).

- [ ] **Step 5: Commit**

```bash
cd /Users/macbookpro/parking_management
git add ui.c
git commit -m "feat: add S_ROLE_SELECT/S_ADMIN_LOGIN states and role fields to ui struct"
```

---

### Task 2: Add load_credentials() and draw_input_masked()

**Files:**
- Modify: `ui.c` — two new static functions, inserted before `ui_init()`

- [ ] **Step 1: Add load_credentials() before ui_init()**

Insert this function immediately before the `// ═══ PUBLIC API` comment block (around line 773):

```c
static void load_credentials(void) {
    strncpy(ui.admin_user, "admin", sizeof(ui.admin_user) - 1);
    strncpy(ui.admin_pass, "1234",  sizeof(ui.admin_pass) - 1);

    FILE *f = fopen("admin.cfg", "r");
    if (!f) {
        f = fopen("admin.cfg", "w");
        if (f) { fprintf(f, "admin\n1234\n"); fclose(f); }
        return;
    }
    char line[64];
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        strncpy(ui.admin_user, line, sizeof(ui.admin_user) - 1);
    }
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        strncpy(ui.admin_pass, line, sizeof(ui.admin_pass) - 1);
    }
    fclose(f);
}
```

- [ ] **Step 2: Add draw_input_masked() after draw_input()**

The existing `draw_input()` ends around line 163. Insert `draw_input_masked()` directly after it:

```c
static void draw_input_masked(Rectangle r, TF *f, int id,
                              const char *label, Vector2 m) {
    if (CheckCollisionPointRec(m, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        ui.active = id;
    int active = (ui.active == id);
    if (label) DrawText(label, (int)r.x, (int)r.y - 18, 12, C_SUB);
    DrawRectangleRec(r, active ? C_INP_A : C_INP);
    DrawRectangleLinesEx(r, 1.5f,
        active ? (Color){80, 140, 255, 255} : C_BOR);
    int len = (int)strlen(f->b);
    char stars[FLEN];
    int i;
    for (i = 0; i < len && i < FLEN - 1; i++) stars[i] = '*';
    stars[i] = '\0';
    DrawText(stars, (int)r.x + 8, (int)(r.y + (r.height - 14) / 2), 14, WHITE);
    if (active && (int)(GetTime() * 2) % 2) {
        int cx = (int)r.x + 8 + MeasureText(stars, 14);
        DrawRectangle(cx, (int)(r.y + (r.height - 14) / 2), 2, 14, WHITE);
    }
}
```

- [ ] **Step 3: Build to verify**

```bash
cd /Users/macbookpro/parking_management && make
```
Expected: compiles with no errors or warnings.

- [ ] **Step 4: Commit**

```bash
cd /Users/macbookpro/parking_management
git add ui.c
git commit -m "feat: add load_credentials() and draw_input_masked() helpers"
```

---

### Task 3: Add draw_role_select() and draw_admin_login()

**Files:**
- Modify: `ui.c` — two new static draw functions, inserted before the STARTUP OVERLAY section

- [ ] **Step 1: Add draw_role_select() before draw_startup()**

The `// STARTUP OVERLAY` comment is around line 730. Insert `draw_role_select()` immediately before it:

```c
static void draw_role_select(Vector2 m) {
    DrawRectangle(0, 0, SCR_W, SCR_H, (Color){0, 0, 0, 190});

    int bw = 500, bh = 180;
    int bx = (SCR_W - bw) / 2, by = (SCR_H - bh) / 2;

    DrawRectangle(bx, by, bw, bh, C_PANEL);
    DrawRectangleLinesEx((Rectangle){bx, by, bw, bh}, 2.0f, C_BLU);
    DrawText("PARKING MANAGEMENT SYSTEM", bx + 20, by + 16, 18, WHITE);
    DrawRectangle(bx + 20, by + 42, bw - 40, 1, C_BOR);
    DrawText("Please select your role to continue", bx + 20, by + 54, 13, C_SUB);

    if (draw_btn((Rectangle){bx + 20,  by + 90, 215, 50}, "ADMINISTRATOR", C_BLU, m)) {
        ui.scr    = S_ADMIN_LOGIN;
        ui.active = 11;
        msg_set("", WHITE);
    }
    if (draw_btn((Rectangle){bx + 255, by + 90, 215, 50}, "USER", C_GRN, m)) {
        ui.role   = 1;
        ui.scr    = S_STARTUP;
        ui.active = 1;
        msg_set("", WHITE);
    }

    DrawText(ui.msg, bx + 20, by + bh - 24, 12, ui.msg_col);
}
```

- [ ] **Step 2: Add draw_admin_login() immediately after draw_role_select()**

```c
static void draw_admin_login(Vector2 m) {
    DrawRectangle(0, 0, SCR_W, SCR_H, (Color){0, 0, 0, 190});

    int bw = 500, bh = 270;
    int bx = (SCR_W - bw) / 2, by = (SCR_H - bh) / 2;

    DrawRectangle(bx, by, bw, bh, C_PANEL);
    DrawRectangleLinesEx((Rectangle){bx, by, bw, bh}, 2.0f, C_BLU);
    DrawText("PARKING MANAGEMENT SYSTEM", bx + 20, by + 16, 18, WHITE);
    DrawRectangle(bx + 20, by + 42, bw - 40, 1, C_BOR);
    draw_heading(bx + 20, by + 54, "Administrator Login");

    draw_input((Rectangle){bx + 20, by + 100, 440, 34},
               &ui.f_username, 11, "Username", m);
    draw_input_masked((Rectangle){bx + 20, by + 162, 440, 34},
                      &ui.f_password, 12, "Password", m);

    if (draw_btn((Rectangle){bx + 20,  by + 210, 200, 42}, "LOGIN", C_GRN, m)
        || IsKeyPressed(KEY_ENTER)) {
        if (strcmp(ui.f_username.b, ui.admin_user) == 0 &&
            strcmp(ui.f_password.b, ui.admin_pass) == 0) {
            ui.role   = 2;
            ui.scr    = S_STARTUP;
            ui.active = 1;
            memset(&ui.f_username, 0, sizeof(TF));
            memset(&ui.f_password, 0, sizeof(TF));
            msg_set("", WHITE);
        } else {
            msg_set("Invalid username or password.", C_RED);
        }
    }
    if (draw_btn((Rectangle){bx + 280, by + 210, 180, 42}, "< BACK", C_INP, m)) {
        ui.scr = S_ROLE_SELECT;
        memset(&ui.f_username, 0, sizeof(TF));
        memset(&ui.f_password, 0, sizeof(TF));
        msg_set("", WHITE);
        ui.active = 0;
    }

    DrawText(ui.msg, bx + 20, by + bh - 24, 12, ui.msg_col);
}
```

- [ ] **Step 3: Build to verify**

```bash
cd /Users/macbookpro/parking_management && make
```
Expected: compiles with no errors. Program still starts at old startup overlay (screens not wired yet).

- [ ] **Step 4: Commit**

```bash
cd /Users/macbookpro/parking_management
git add ui.c
git commit -m "feat: add draw_role_select() and draw_admin_login() screens"
```

---

### Task 4: Wire new screens into ui_init() and ui_draw()

**Files:**
- Modify: `ui.c` — `ui_init()` and `ui_draw()`

- [ ] **Step 1: Update ui_init() to start at S_ROLE_SELECT and load credentials**

Find `ui_init()`. It currently contains:
```c
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
```
Replace with:
```c
void ui_init(void) {
    memset(&ui, 0, sizeof(ui));
    ui.scr        = S_ROLE_SELECT;
    ui.active     = 0;
    ui.sel_slot   = -1;
    ui.search_idx = -1;
    ui.upd_size   = 0;
    ui.msg_col    = WHITE;

    load_credentials();

    FILE *f = fopen(DATA_FILE, "r");
    if (f) { fclose(f); ui.has_file = 1; }
}
```

- [ ] **Step 2: Update the pre-started branch in ui_draw()**

Find this block in `ui_draw()`:
```c
    if (!ui.started) {
        draw_title_bar();
        draw_startup(m);
        return;
    }
```
Replace with:
```c
    if (!ui.started) {
        draw_title_bar();
        switch (ui.scr) {
            case S_ROLE_SELECT: draw_role_select(m); break;
            case S_ADMIN_LOGIN: draw_admin_login(m); break;
            default:            draw_startup(m);     break;
        }
        return;
    }
```

- [ ] **Step 3: Build and run — verify role-selection screen appears**

```bash
cd /Users/macbookpro/parking_management && make && ./parking_gui
```
Expected:
- Program opens to a dark overlay with "PARKING MANAGEMENT SYSTEM" title and two buttons: ADMINISTRATOR / USER
- Clicking USER opens the existing startup overlay (lot size / load file)
- Clicking ADMINISTRATOR opens a login form with Username and Password fields
- Password field shows `***` when typed
- Entering wrong credentials shows "Invalid username or password." in red
- Entering `admin` / `1234` (or whatever is in `admin.cfg`) proceeds to the startup overlay
- `< BACK` button on the login screen returns to role selection

- [ ] **Step 4: Commit**

```bash
cd /Users/macbookpro/parking_management
git add ui.c
git commit -m "feat: wire role-select and admin-login screens into ui_init and ui_draw"
```

---

### Task 5: Role-based nav filtering and logout button

**Files:**
- Modify: `ui.c` — `draw_nav()`

- [ ] **Step 1: Replace draw_nav() with role-aware version**

Find the entire `draw_nav()` function (starts around line 308). Replace it completely with:

```c
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

    int row = 0;
    for (int i = 0; i < 8; i++) {
        if (ui.role != 2 && (screens[i] == S_ADD || screens[i] == S_FILE))
            continue;
        Rectangle r = {5, TOP_H + 8 + row * 52, NAV_W - 10, 44};
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
        row++;
    }

    Rectangle lr = {5, SCR_H - 52, NAV_W - 10, 44};
    if (draw_btn(lr, "LOGOUT", C_RED, m)) {
        ui.role    = 0;
        ui.started = 0;
        ui.scr     = S_ROLE_SELECT;
        ui.active  = 0;
        ui.sel_slot= -1;
        ui.scroll  = 0;
        ui.lscroll = 0;
        msg_set("", WHITE);
    }
}
```

- [ ] **Step 2: Build and run — verify role-based nav**

```bash
cd /Users/macbookpro/parking_management && make && ./parking_gui
```
Expected when logged in as **User**:
- Nav shows 6 items: View All, Park Vehicle, Remove Vehicle, Update Info, Search, Summary
- "Add Slots" and "Save / Load" are not visible
- LOGOUT button (red) appears at the bottom of the nav

Expected when logged in as **Admin**:
- Nav shows all 8 items including Add Slots and Save / Load
- LOGOUT button (red) appears at the bottom of the nav

- [ ] **Step 3: Commit**

```bash
cd /Users/macbookpro/parking_management
git add ui.c
git commit -m "feat: filter nav by role and add logout button"
```

---

### Task 6: Integration test all paths

No code changes. Verify all flows manually.

- [ ] **Step 1: Build a clean binary**

```bash
cd /Users/macbookpro/parking_management && make clean && make && ./parking_gui
```

- [ ] **Step 2: Test — wrong admin credentials**
  - Click ADMINISTRATOR
  - Enter wrong username or password, click LOGIN
  - Verify red message: "Invalid username or password."
  - Verify form stays open

- [ ] **Step 3: Test — admin login and full nav**
  - Click ADMINISTRATOR
  - Enter `admin` / `1234`, click LOGIN
  - Verify startup overlay appears
  - Create a lot or load file
  - Verify all 8 nav items are visible
  - Verify LOGOUT (red) button is at the bottom of the nav

- [ ] **Step 4: Test — user login and restricted nav**
  - Click LOGOUT (returns to role select)
  - Click USER
  - Complete startup (create lot or load file)
  - Verify only 6 nav items are visible (no Add Slots, no Save / Load)
  - Verify LOGOUT button is still present

- [ ] **Step 5: Test — logout and re-login**
  - While logged in as User, click LOGOUT
  - Verify role-selection screen appears
  - Log in as Admin
  - Verify all 8 nav items appear (no items from prior session bleed through)

- [ ] **Step 6: Test — admin.cfg editing**
  - Close the program
  - Open `admin.cfg` in a text editor, change to:
    ```
    manager
    secret99
    ```
  - Restart program, try logging in with `admin` / `1234` — should fail
  - Log in with `manager` / `secret99` — should succeed

- [ ] **Step 7: Test — admin.cfg auto-creation**
  - Delete `admin.cfg`
  - Run the program
  - Verify `admin.cfg` is recreated with `admin` / `1234`
  - Verify login works with those defaults

- [ ] **Step 8: Final commit**

```bash
cd /Users/macbookpro/parking_management
git add ui.c
git commit -m "feat: complete admin/user role system with login, role-based nav, and logout"
```
