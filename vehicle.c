// vehicle.c — park, remove, update vehicles; fee calculation

#include <string.h>
#include <stdio.h>
#include "parking.h"

double calc_fee(int size, int hours) {
    double rates[] = { 1.0, 2.0, 3.0 };   // small, medium, big
    return hours * rates[size];
}

int park_car(int slot_id, const char *plate, const char *type,
             int size, int entry, char *err) {
    if (!plate || !plate[0])       { sprintf(err, "Plate is empty");        return 0; }
    if (size < 0 || size > 2)      { sprintf(err, "Pick a size");           return 0; }
    if (entry < 0 || entry > 23)   { sprintf(err, "Entry hour must be 0-23"); return 0; }

    int i = find_slot(slot_id);
    if (i < 0)                     { sprintf(err, "Slot not found");        return 0; }
    if (lot[i].occupied)           { sprintf(err, "Slot already occupied"); return 0; }
    if (find_plate(plate) >= 0)    { sprintf(err, "Plate already parked");  return 0; }

    strncpy(lot[i].v.plate, plate,                       MAX_PLATE - 1);
    strncpy(lot[i].v.type, (type && type[0]) ? type : "Unknown", MAX_TYPE - 1);
    lot[i].v.size       = size;
    lot[i].v.entry_hour = entry;
    lot[i].v.exit_hour  = -1;
    lot[i].occupied     = 1;
    return 1;
}

int remove_car(int slot_id, int exit_h,
               double *fee_out, int *hours_out, char *err) {
    if (exit_h < 0 || exit_h > 23) { sprintf(err, "Exit hour must be 0-23"); return 0; }

    int i = find_slot(slot_id);
    if (i < 0)            { sprintf(err, "Slot not found"); return 0; }
    if (!lot[i].occupied) { sprintf(err, "Slot is empty");  return 0; }

    int hours = exit_h - lot[i].v.entry_hour;
    if (hours <= 0) hours = 1;           // minimum 1 hour charge
    double fee = calc_fee(lot[i].v.size, hours);

    *fee_out   = fee;
    *hours_out = hours;
    income    += fee;

    lot[i].occupied = 0;
    memset(&lot[i].v, 0, sizeof(struct Vehicle));
    lot[i].v.exit_hour = -1;
    return 1;
}

int update_car(int slot_id, const char *plate, const char *type,
               int size, char *err) {
    int i = find_slot(slot_id);
    if (i < 0)            { sprintf(err, "Slot not found"); return 0; }
    if (!lot[i].occupied) { sprintf(err, "Slot is empty");  return 0; }

    // only update plate if it actually changed (to avoid false "plate in use" error)
    if (plate && plate[0] && strcmp(plate, lot[i].v.plate) != 0) {
        if (find_plate(plate) >= 0) { sprintf(err, "Plate already in use"); return 0; }
        strncpy(lot[i].v.plate, plate, MAX_PLATE - 1);
    }
    if (type && type[0]) strncpy(lot[i].v.type, type, MAX_TYPE - 1);
    if (size >= 0 && size <= 2) lot[i].v.size = size;
    return 1;
}
