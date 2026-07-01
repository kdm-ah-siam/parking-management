// file-saving.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function-define.h"

int save_file(char *err) {
    char tmp[64] = "parking_data.txt.tmp";
    FILE *f = fopen(tmp, "w");
    if (!f) {
        strcpy(err, "Cannot open file for writing");
        return 0;
    }
    fprintf(f, "%d %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
            num_slots, income,
            fee_rates[0], fee_rates[1], fee_rates[2],
            daily_cap[0], daily_cap[1], daily_cap[2]);
    for (int i = 0; i < num_slots; i++) {
        if (lot[i].occupied) {
            fprintf(f, "%d 1 %s %s %d %d %d %d %d %d %d %d %d\n",
                    lot[i].id, lot[i].v.plate, lot[i].v.type, lot[i].v.size,
                    lot[i].v.entry_hour, lot[i].v.entry_min, lot[i].v.entry_sec,
                    lot[i].v.exit_hour,
                    lot[i].v.entry_day, lot[i].v.entry_month, lot[i].v.entry_year,
                    lot[i].size);
        } else {
            fprintf(f, "%d 0 %d\n", lot[i].id, lot[i].size);
        }
    }
    fclose(f);
    if (rename(tmp, DATA_FILE) != 0) {
        strcpy(err, "Failed to save file");
        remove(tmp);
        return 0;
    }
    return 1;
}

int load_file(char *err) {
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) {
        strcpy(err, "File not found");
        return 0;
    }

    int n = 0;
    double inc = 0.0;
    int hdr = fscanf(f, "%d %lf", &n, &inc);
    if (hdr != 2 || n <= 0 || n > 5000) {
        strcpy(err, "Bad file format");
        fclose(f);
        return 0;
    }

    fscanf(f, " %lf %lf %lf %lf %lf %lf",
        &fee_rates[0], &fee_rates[1], &fee_rates[2],
        &daily_cap[0], &daily_cap[1], &daily_cap[2]);
        
    init_lot(n);
    income = inc;

    for (int i = 0; i < n; i++) {
        int id = 0;
        int occ = 0;
        if (fscanf(f, "%d %d", &id, &occ) != 2) break;
        lot[i].id = id;
        lot[i].occupied = occ;
        if (occ) {
            int slot_size = 1;
            if (fscanf(f, " %19s %19s %d %d %d %d %d %d %d %d %d",
                       lot[i].v.plate, lot[i].v.type, &lot[i].v.size,
                       &lot[i].v.entry_hour, &lot[i].v.entry_min, &lot[i].v.entry_sec,
                       &lot[i].v.exit_hour,
                       &lot[i].v.entry_day, &lot[i].v.entry_month, &lot[i].v.entry_year,
                       &slot_size) != 11) {
                memset(&lot[i].v, 0, sizeof(lot[i].v));
                lot[i].v.exit_hour = -1;
                lot[i].occupied = 0;
            }
            lot[i].size = slot_size;
        } else {
            int slot_size = 1;
            fscanf(f, " %d", &slot_size);
            lot[i].size = slot_size;
        }
    }
    fclose(f);
    return 1;
}

int append_report(int year, int month, int day,
                  int entry_h, int exit_h, const char *plate, const char *type,
                  int size, int hours, double fee, char *err) {
    FILE *f = fopen(REPORT_FILE, "a");
    if (!f) {
        strcpy(err, "Cannot open reports file");
        return 0;
    }
    fprintf(f, "%04d-%02d-%02d %d %d %s %s %d %d %.2f\n",
            year, month, day, entry_h, exit_h, plate, type, size, hours, fee);
    fclose(f);
    return 1;
}

static int parse_line(const char *line, struct Transaction *t) {
    char date[16];
    if (sscanf(line, "%15s %d %d %19s %19s %d %d %lf",
               date, &t->entry_h, &t->exit_h, t->plate, t->type,
               &t->size, &t->hours, &t->fee) != 8) {
        return 0;
    }
    return sscanf(date, "%d-%d-%d", &t->year, &t->month, &t->day) == 3;
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
