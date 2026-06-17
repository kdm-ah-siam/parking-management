// vehicle.c — park, remove, update vehicles and fee calculation

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parking.h"

double calc_fee(int size, int hours) {
    return hours * fee_rates[size];
}

int park_car(int slot_id, const char *plate, const char *type,
             int size, int entry, char *err) {
    if (!plate || !plate[0])     { strcpy(err, "Plate is empty");          return 0; }
    if (size < 0 || size > 2)    { strcpy(err, "Pick a size");             return 0; }
    if (entry < 0 || entry > 23) { strcpy(err, "Entry hour must be 0-23"); return 0; }

    // uppercase the plate and reject invalid characters
    char up[MAX_PLATE] = {0};
    for (int j = 0; plate[j] && j < MAX_PLATE - 1; j++) {
        char c = plate[j];
        if (c >= 'a' && c <= 'z') c -= 32;
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-')) {
            strcpy(err, "Plate: letters, numbers, hyphens only");
            return 0;
        }
        up[j] = c;
    }

    int i = find_slot(slot_id);
    if (i < 0)               { strcpy(err, "Slot not found");       return 0; }
    if (lot[i].occupied)     { strcpy(err, "Slot already occupied"); return 0; }
    if (find_plate(up) >= 0) { strcpy(err, "Plate already parked");  return 0; }

    strncpy(lot[i].v.plate, up, MAX_PLATE - 1);
    strncpy(lot[i].v.type, (type && type[0]) ? type : "Unknown", MAX_TYPE - 1);
    lot[i].v.size       = size;
    lot[i].v.entry_hour = entry;
    lot[i].v.exit_hour  = -1;

    time_t now = time(NULL);
    struct tm *td = localtime(&now);
    lot[i].v.entry_day   = td->tm_mday;
    lot[i].v.entry_month = td->tm_mon + 1;
    lot[i].v.entry_year  = td->tm_year + 1900;
    lot[i].occupied = 1;
    return 1;
}

int remove_car(int slot_id, int exit_h, double *fee_out, int *hours_out, char *err) {
    if (exit_h < 0 || exit_h > 23) { strcpy(err, "Exit hour must be 0-23"); return 0; }
    int i = find_slot(slot_id);
    if (i < 0)            { strcpy(err, "Slot not found"); return 0; }
    if (!lot[i].occupied) { strcpy(err, "Slot is empty");  return 0; }

    struct Vehicle *v = &lot[i].v;

    // build entry timestamp
    struct tm t = {0};
    t.tm_year  = v->entry_year - 1900;
    t.tm_mon   = v->entry_month - 1;
    t.tm_mday  = v->entry_day;
    t.tm_hour  = v->entry_hour;
    t.tm_isdst = -1;
    time_t entry_t = mktime(&t);

    // build exit timestamp (today's date at exit_h)
    time_t now_t = time(NULL);
    struct tm *today = localtime(&now_t);
    t = *today;
    t.tm_hour  = exit_h;
    t.tm_min   = 0;
    t.tm_sec   = 0;
    t.tm_isdst = -1;
    time_t exit_t = mktime(&t);

    int hours = (int)(difftime(exit_t, entry_t) / 3600.0);
    if (hours <= 0) hours = 1;

    double fee = calc_fee(v->size, hours);
    *fee_out   = fee;
    *hours_out = hours;
    income    += fee;

    char rerr[128] = "";
    append_report(today->tm_year + 1900, today->tm_mon + 1, today->tm_mday,
                  v->entry_hour, exit_h, v->plate, v->type, v->size, hours, fee, rerr);

    memset(v, 0, sizeof(struct Vehicle));
    v->exit_hour    = -1;
    lot[i].occupied = 0;
    return 1;
}

int update_car(int slot_id, const char *plate, const char *type, int size, char *err) {
    int i = find_slot(slot_id);
    if (i < 0)            { strcpy(err, "Slot not found"); return 0; }
    if (!lot[i].occupied) { strcpy(err, "Slot is empty");  return 0; }

    if (plate && plate[0] && strcmp(plate, lot[i].v.plate) != 0) {
        if (find_plate(plate) >= 0) { strcpy(err, "Plate already in use"); return 0; }
        strncpy(lot[i].v.plate, plate, MAX_PLATE - 1);
    }
    if (type && type[0]) strncpy(lot[i].v.type, type, MAX_TYPE - 1);
    if (size >= 0 && size <= 2) lot[i].v.size = size;
    return 1;
}
