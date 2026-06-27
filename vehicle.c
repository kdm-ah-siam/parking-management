// vehicle.c

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "parking.h"

double calc_fee(int size, int hours) {
    double fee = hours * fee_rates[size];
    if (daily_cap[size] > 0.0 && fee > daily_cap[size]) {
        fee = daily_cap[size];
    }
    return fee;
}

time_t vehicle_entry_time(const struct Vehicle *v) {
    struct tm t = {0};
    t.tm_year  = v->entry_year - 1900;
    t.tm_mon   = v->entry_month - 1;
    t.tm_mday  = v->entry_day;
    t.tm_hour  = v->entry_hour;
    t.tm_min   = v->entry_min;
    t.tm_sec   = v->entry_sec;
    t.tm_isdst = -1;
    return mktime(&t);
}

// converts plate to uppercase and checks for valid characters
static int normalize_plate(const char *in, char *out) {
    int j = 0;
    while (in[j] && j < MAX_PLATE - 1) {
        char c = toupper(in[j]);
        if (!isalpha(c) && !isdigit(c) && c != '-') return 0;
        out[j] = c;
        j++;
    }
    out[j] = '\0';
    return 1;
}

int park_car(int slot_id, const char *plate, const char *type, int size, char *err) {
    if (!plate || !plate[0]) {
        strcpy(err, "Plate is empty");
        return 0;
    }
    if (size < 0 || size > 2) {
        strcpy(err, "Pick a size");
        return 0;
    }

    char up[MAX_PLATE] = {0};
    if (!normalize_plate(plate, up)) {
        strcpy(err, "Plate: letters, numbers, hyphens only");
        return 0;
    }

    int i = find_slot(slot_id);
    if (i < 0) {
        strcpy(err, "Slot not found");
        return 0;
    }
    if (lot[i].occupied) {
        strcpy(err, "Slot already occupied");
        return 0;
    }
    if (lot[i].size != size) {
        strcpy(err, "Slot size mismatch");
        return 0;
    }
    if (find_plate(up) >= 0) {
        strcpy(err, "Plate already parked");
        return 0;
    }

    strncpy(lot[i].v.plate, up, MAX_PLATE - 1);

    if (type && type[0]) {
        strncpy(lot[i].v.type, type, MAX_TYPE - 1);
    } else {
        strncpy(lot[i].v.type, "Unknown", MAX_TYPE - 1);
    }

    lot[i].v.size      = size;
    lot[i].v.exit_hour = -1;

    time_t now = time(NULL);
    struct tm *td = localtime(&now);
    lot[i].v.entry_hour  = td->tm_hour;
    lot[i].v.entry_min   = td->tm_min;
    lot[i].v.entry_sec   = td->tm_sec;
    lot[i].v.entry_day   = td->tm_mday;
    lot[i].v.entry_month = td->tm_mon + 1;
    lot[i].v.entry_year  = td->tm_year + 1900;
    lot[i].occupied = 1;
    return 1;
}

int remove_car(int slot_id, double *fee_out, int *hours_out, char *err) {
    int i = find_slot(slot_id);
    if (i < 0) {
        strcpy(err, "Slot not found");
        return 0;
    }
    if (!lot[i].occupied) {
        strcpy(err, "Slot is empty");
        return 0;
    }

    struct Vehicle *v = &lot[i].v;

    time_t entry_t = vehicle_entry_time(v);
    time_t now_t = time(NULL);
    struct tm today = *localtime(&now_t);

    int hours = (int)(difftime(now_t, entry_t) / 3600.0);
    if (hours <= 0) hours = 1;

    double fee = calc_fee(v->size, hours);
    *fee_out   = fee;
    *hours_out = hours;
    income += fee;

    char rerr[128] = "";
    if (!append_report(v->entry_year, v->entry_month, v->entry_day,
                       v->entry_hour, today.tm_hour, v->plate, v->type, v->size, hours, fee, rerr)) {
        fprintf(stderr, "report write failed: %s\n", rerr);
    }

    memset(v, 0, sizeof(struct Vehicle));
    v->exit_hour = -1;
    lot[i].occupied = 0;
    return 1;
}
