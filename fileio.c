// fileio.c — save and load parking_data.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parking.h"

int save_file(char *err) {
    FILE *f = fopen(DATA_FILE, "w");
    if (!f) { sprintf(err, "Cannot open file for writing"); return 0; }

    fprintf(f, "%d %.2f\n", num_slots, income);
    for (int i = 0; i < num_slots; i++) {
        if (lot[i].occupied)
            fprintf(f, "%d 1 %s %s %d %d %d\n",
                    lot[i].id,
                    lot[i].v.plate, lot[i].v.type,
                    lot[i].v.size,
                    lot[i].v.entry_hour, lot[i].v.exit_hour);
        else
            fprintf(f, "%d 0\n", lot[i].id);
    }
    fclose(f);
    return 1;
}

int load_file(char *err) {
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) { sprintf(err, "File not found"); return 0; }

    int n; double inc;
    if (fscanf(f, "%d %lf\n", &n, &inc) != 2 || n <= 0) {
        sprintf(err, "Bad file format"); fclose(f); return 0;
    }

    init_lot(n);
    income = inc;

    for (int i = 0; i < n; i++) {
        int id, occ;
        if (fscanf(f, "%d %d", &id, &occ) != 2) break;
        lot[i].id       = id;
        lot[i].occupied = occ;
        if (occ)
            fscanf(f, " %s %s %d %d %d",
                   lot[i].v.plate, lot[i].v.type,
                   &lot[i].v.size,
                   &lot[i].v.entry_hour, &lot[i].v.exit_hour);
    }
    fclose(f);
    return 1;
}
