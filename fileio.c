// fileio.c — save and load parking_data.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parking.h"

int save_file(char *err)
{
    FILE *f = fopen(DATA_FILE, "w");
    if (f == NULL)
    {
        sprintf(err, "Cannot open file for writing");
        return 0;
    }

    fprintf(f, "%d %.2f\n", num_slots, income);

    for (int i = 0; i < num_slots; i++)
    {
        if (lot[i].occupied == 1)
        {
            fprintf(f, "%d 1 %s %s %d %d %d %d %d %d\n",
                    lot[i].id,
                    lot[i].v.plate, lot[i].v.type,
                    lot[i].v.size,
                    lot[i].v.entry_hour, lot[i].v.exit_hour,
                    lot[i].v.entry_day, lot[i].v.entry_month,
                    lot[i].v.entry_year);
        }
        else
        {
            fprintf(f, "%d 0\n", lot[i].id);
        }
    }

    fclose(f);
    return 1;
}

int load_file(char *err)
{
    FILE *f = fopen(DATA_FILE, "r");
    if (f == NULL)
    {
        sprintf(err, "File not found");
        return 0;
    }

    int n = 0;
    double inc = 0.0;
    int read_ok = fscanf(f, "%d %lf\n", &n, &inc);

    if (read_ok != 2 || n <= 0)
    {
        sprintf(err, "Bad file format");
        fclose(f);
        return 0;
    }

    init_lot(n);
    income = inc;

    for (int i = 0; i < n; i++)
    {
        int id  = 0;
        int occ = 0;
        if (fscanf(f, "%d %d", &id, &occ) != 2)
        {
            break;
        }
        lot[i].id       = id;
        lot[i].occupied = occ;
        if (occ == 1)
        {
            fscanf(f, " %s %s %d %d %d %d %d %d",
                   lot[i].v.plate, lot[i].v.type,
                   &lot[i].v.size,
                   &lot[i].v.entry_hour, &lot[i].v.exit_hour,
                   &lot[i].v.entry_day, &lot[i].v.entry_month,
                   &lot[i].v.entry_year);
        }
    }

    fclose(f);
    return 1;
}

int append_report(int year, int month, int day,
                  int entry_h, int exit_h,
                  const char *plate, const char *type,
                  int size, int hours, double fee, char *err)
{
    FILE *f = fopen(REPORT_FILE, "a");
    if (f == NULL)
    {
        sprintf(err, "Cannot open reports file");
        return 0;
    }
    fprintf(f, "%04d-%02d-%02d %d %d %s %s %d %d %.2f\n",
            year, month, day, entry_h, exit_h,
            plate, type, size, hours, fee);
    fclose(f);
    return 1;
}

int read_reports(int year, int month, int day,
                 struct Transaction *out, int max_out)
{
    FILE *f = fopen(REPORT_FILE, "r");
    if (f == NULL)
    {
        return 0;
    }

    int count = 0;
    char line[256];

    while (fgets(line, sizeof(line), f) != NULL && count < max_out)
    {
        struct Transaction t;
        char date_str[16];

        int parsed = sscanf(line, "%15s %d %d %19s %19s %d %d %lf",
                            date_str,
                            &t.entry_h, &t.exit_h,
                            t.plate, t.type,
                            &t.size, &t.hours, &t.fee);

        if (parsed != 8)
        {
            continue;
        }

        if (sscanf(date_str, "%d-%d-%d", &t.year, &t.month, &t.day) != 3)
        {
            continue;
        }

        if (t.year != year || t.month != month)
        {
            continue;
        }

        if (day > 0 && t.day != day)
        {
            continue;
        }

        out[count] = t;
        count++;
    }

    fclose(f);
    return count;
}
