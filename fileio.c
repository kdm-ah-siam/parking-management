// fileio.c — save/load parking_data.txt and read/write reports.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parking.h"

int save_file(char *err) {
    FILE *f = fopen(DATA_FILE, "w");
    if (!f) { strcpy(err, "Cannot open file for writing"); return 0; }
    fprintf(f, "%d %.2f\n", num_slots, income);
    for (int i = 0; i < num_slots; i++) {
        if (lot[i].occupied)
            fprintf(f, "%d 1 %s %s %d %d %d %d %d %d %d %d\n",
                    lot[i].id, lot[i].v.plate, lot[i].v.type, lot[i].v.size,
                    lot[i].v.entry_hour, lot[i].v.entry_min, lot[i].v.entry_sec,
                    lot[i].v.exit_hour,
                    lot[i].v.entry_day, lot[i].v.entry_month, lot[i].v.entry_year);
        else
            fprintf(f, "%d 0\n", lot[i].id);
    }
    fclose(f);
    return 1;
}

int load_file(char *err) {
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) { strcpy(err, "File not found"); return 0; }
    int n = 0; double inc = 0.0;
    if (fscanf(f, "%d %lf\n", &n, &inc) != 2 || n <= 0) {
        strcpy(err, "Bad file format"); fclose(f); return 0;
    }
    init_lot(n);
    income = inc;
    for (int i = 0; i < n; i++) {
        int id = 0, occ = 0;
        if (fscanf(f, "%d %d", &id, &occ) != 2) break;
        lot[i].id = id;
        lot[i].occupied = occ;
        if (occ)
            fscanf(f, " %19s %19s %d %d %d %d %d %d %d %d",
                   lot[i].v.plate, lot[i].v.type, &lot[i].v.size,
                   &lot[i].v.entry_hour, &lot[i].v.entry_min, &lot[i].v.entry_sec,
                   &lot[i].v.exit_hour,
                   &lot[i].v.entry_day, &lot[i].v.entry_month, &lot[i].v.entry_year);
    }
    fclose(f);
    return 1;
}

int append_report(int year, int month, int day,
                  int entry_h, int exit_h, const char *plate, const char *type,
                  int size, int hours, double fee, char *err) {
    FILE *f = fopen(REPORT_FILE, "a");
    if (!f) { strcpy(err, "Cannot open reports file"); return 0; }
    fprintf(f, "%04d-%02d-%02d %d %d %s %s %d %d %.2f\n",
            year, month, day, entry_h, exit_h, plate, type, size, hours, fee);
    fclose(f);
    return 1;
}

// parse one line from reports.txt into a Transaction; returns 1 on success
static int parse_line(const char *line, struct Transaction *t) {
    char date[16];
    if (sscanf(line, "%15s %d %d %19s %19s %d %d %lf",
               date, &t->entry_h, &t->exit_h, t->plate, t->type,
               &t->size, &t->hours, &t->fee) != 8) return 0;
    return sscanf(date, "%d-%d-%d", &t->year, &t->month, &t->day) == 3;
}

// case-insensitive plate comparison
static int plate_match(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
        char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == *b;
}

int read_reports(int year, int month, int day, struct Transaction *out, int max) {
    FILE *f = fopen(REPORT_FILE, "r");
    if (!f) return 0;
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && count < max) {
        struct Transaction t;
        if (!parse_line(line, &t)) continue;
        if (t.year != year || t.month != month) continue;
        if (day > 0 && t.day != day) continue;
        out[count++] = t;
    }
    fclose(f);
    return count;
}

int read_reports_by_plate(const char *plate, struct Transaction *out, int max) {
    FILE *f = fopen(REPORT_FILE, "r");
    if (!f) return 0;
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && count < max) {
        struct Transaction t;
        if (!parse_line(line, &t)) continue;
        if (!plate_match(plate, t.plate)) continue;
        out[count++] = t;
    }
    fclose(f);
    return count;
}
