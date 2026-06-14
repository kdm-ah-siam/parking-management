# Admin / User Role System — Design Spec
**Date:** 2026-06-14
**Project:** Parking Management System (C / Raylib)

---

## Overview

Add a role-selection login screen at program startup. Users choose between Administrator and User. Administrators authenticate with username + password; Users go straight to the parking program with a restricted feature set. A logout button returns either role to the selection screen.

---

## Screen Flow

```
Program starts
     │
     ▼
S_ROLE_SELECT  ◄──── Logout (any time, from nav)
  [Administrator]  [User]
     │                  │
     ▼                  ▼
S_ADMIN_LOGIN      S_STARTUP (existing lot-size / load dialog)
  username + pass        │
     │ (success)         ▼
     ▼           Nav: View All, Park, Remove,
  S_STARTUP       Update, Search, Summary
     │            (Add Slots and Save/Load hidden)
     ▼
Nav: all 8 items
```

---

## New Screen States

Two entries added to the `Screen` enum in `ui.c`:

| State | Purpose |
|---|---|
| `S_ROLE_SELECT` | First screen — two buttons: Administrator / User |
| `S_ADMIN_LOGIN` | Username + password form with Login and Back buttons |

`ui_init()` sets `ui.scr = S_ROLE_SELECT` (was `S_STARTUP`).

---

## Credentials

**File:** `admin.cfg` (plain text, two lines)
```
admin
1234
```

- Loaded once in `ui_init()` into `ui.admin_user` and `ui.admin_pass`.
- If the file does not exist, defaults (`admin` / `1234`) are used and the file is created automatically so the admin can edit it outside the app.
- No encryption — suitable for a local desktop utility.

---

## UI State Changes

New fields added to the anonymous `ui` struct in `ui.c`:

```c
int  role;            // 0=none, 1=user, 2=admin
TF   f_username;      // text field id 11
TF   f_password;      // text field id 12 (masked display)
char admin_user[64];  // loaded from admin.cfg
char admin_pass[64];  // loaded from admin.cfg
```

`tf()` switch extended with cases 11 and 12.

---

## Password Masking

`draw_input()` gains an extra `int masked` parameter. When `masked=1`, the function draws `*` characters equal to `strlen(f->b)` instead of the real buffer. Typing and backspace still operate on the real buffer. Only `f_password` (id 12) uses `masked=1`.

---

## Role-Based Nav

`draw_nav()` conditionally renders nav items based on `ui.role`:

| Nav Item | User | Admin |
|---|---|---|
| View All | yes | yes |
| Park Vehicle | yes | yes |
| Remove Vehicle | yes | yes |
| Update Info | yes | yes |
| Search | yes | yes |
| Summary | yes | yes |
| Add Slots | — | yes |
| Save / Load | — | yes |

A **Logout** button is rendered at the bottom of the nav panel (`C_RED` background) whenever `ui.started` is true. Clicking it:
1. Sets `ui.role = 0`, `ui.started = 0`
2. Clears credentials fields
3. Sets `ui.scr = S_ROLE_SELECT`

---

## New Screen Layouts

All new screens use the same dark modal overlay style as the existing `draw_startup()`.

**S_ROLE_SELECT:**
```
┌─────────────────────────────────────────┐
│  PARKING MANAGEMENT SYSTEM              │
│ ─────────────────────────────────────── │
│  Please select your role to continue    │
│                                         │
│  [ ADMINISTRATOR ]     [    USER    ]   │
│                                         │
│  <status/error message>                 │
└─────────────────────────────────────────┘
```

**S_ADMIN_LOGIN:**
```
┌─────────────────────────────────────────┐
│  PARKING MANAGEMENT SYSTEM              │
│ ─────────────────────────────────────── │
│  Administrator Login                    │
│                                         │
│  Username  [ _________________ ]        │
│  Password  [ *** _____________ ]        │
│                                         │
│  [    LOGIN    ]     [  ← Back  ]       │
│  (Back returns to S_ROLE_SELECT)        │
│                                         │
│  <wrong credentials message>            │
└─────────────────────────────────────────┘
```

All buttons reuse `draw_btn()`. All inputs reuse `draw_input()`. No new draw primitives needed.

---

## Files Changed

| File | Change |
|---|---|
| `ui.c` | All new logic — new states, new screens, role filtering, logout, credential loading |
| `parking.h` | No change |
| `ui.h` | No change |
| `main.c` | No change |
| `admin.cfg` | New file (auto-created if missing) |

---

## Out of Scope

- In-app password change screen
- Multiple admin accounts
- Session timeout
- Password hashing / encryption
