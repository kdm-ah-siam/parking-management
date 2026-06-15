// vehicle.c — park, remove, update vehicles; fee calculation

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parking.h"

double calc_fee(int size, int hours)
{
    return hours * fee_rates[size];
}

int park_car(int slot_id, const char *plate, const char *type,
             int size, int entry, char *err)
{
    // Validate inputs
    if (plate == NULL || plate[0] == '\0')
    {
        sprintf(err, "Plate is empty");
        return 0;
    }
    if (size < 0 || size > 2)
    {
        sprintf(err, "Pick a size");
        return 0;
    }
    if (entry < 0 || entry > 23)
    {
        sprintf(err, "Entry hour must be 0-23");
        return 0;
    }

    // Validate plate — only letters, numbers, hyphens; auto-uppercase
    char up[MAX_PLATE] = {0};
    for (int j = 0; plate[j] != '\0' && j < MAX_PLATE - 1; j++)
    {
        char c = plate[j];
        if (c >= 'a' && c <= 'z')
        {
            c = c - 32;
        }
        int is_letter = (c >= 'A' && c <= 'Z');
        int is_digit  = (c >= '0' && c <= '9');
        int is_hyphen = (c == '-');
        if (is_letter == 0 && is_digit == 0 && is_hyphen == 0)
        {
            sprintf(err, "Plate: letters, numbers, hyphens only");
            return 0;
        }
        up[j] = c;
    }

    int i = find_slot(slot_id);
    if (i < 0)
    {
        sprintf(err, "Slot not found");
        return 0;
    }
    if (lot[i].occupied == 1)
    {
        sprintf(err, "Slot already occupied");
        return 0;
    }
    if (find_plate(up) >= 0)
    {
        sprintf(err, "Plate already parked");
        return 0;
    }

    // Store vehicle data
    strncpy(lot[i].v.plate, up, MAX_PLATE - 1);

    if (type != NULL && type[0] != '\0')
    {
        strncpy(lot[i].v.type, type, MAX_TYPE - 1);
    }
    else
    {
        strncpy(lot[i].v.type, "Unknown", MAX_TYPE - 1);
    }

    lot[i].v.size      = size;
    lot[i].v.entry_hour = entry;
    lot[i].v.exit_hour  = -1;

    // Capture today's date from the system clock
    time_t now = time(NULL);
    struct tm *td = localtime(&now);
    lot[i].v.entry_day   = td->tm_mday;
    lot[i].v.entry_month = td->tm_mon + 1;
    lot[i].v.entry_year  = td->tm_year + 1900;

    lot[i].occupied = 1;
    return 1;
}

int remove_car(int slot_id, int exit_h,
               double *fee_out, int *hours_out, char *err)
{
    if (exit_h < 0 || exit_h > 23)
    {
        sprintf(err, "Exit hour must be 0-23");
        return 0;
    }

    int i = find_slot(slot_id);
    if (i < 0)
    {
        sprintf(err, "Slot not found");
        return 0;
    }
    if (lot[i].occupied == 0)
    {
        sprintf(err, "Slot is empty");
        return 0;
    }

    // Build a timestamp for when the car entered
    struct tm entry_tm = {0};
    entry_tm.tm_year  = lot[i].v.entry_year - 1900;
    entry_tm.tm_mon   = lot[i].v.entry_month - 1;
    entry_tm.tm_mday  = lot[i].v.entry_day;
    entry_tm.tm_hour  = lot[i].v.entry_hour;
    entry_tm.tm_min   = 0;
    entry_tm.tm_sec   = 0;
    entry_tm.tm_isdst = -1;
    time_t entry_t = mktime(&entry_tm);

    // Build a timestamp for the exit time (today's date, at exit_h)
    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    struct tm exit_tm = *today;
    exit_tm.tm_hour  = exit_h;
    exit_tm.tm_min   = 0;
    exit_tm.tm_sec   = 0;
    exit_tm.tm_isdst = -1;
    time_t exit_t = mktime(&exit_tm);

    // Compute elapsed hours (handles overnight stays correctly)
    int hours = (int)(difftime(exit_t, entry_t) / 3600.0);
    if (hours <= 0)
    {
        hours = 1;  // minimum 1 hour charge
    }

    double fee = calc_fee(lot[i].v.size, hours);

    *fee_out   = fee;
    *hours_out = hours;
    income     = income + fee;

    // Log the transaction to the reports file
    char rerr[128] = "";
    append_report(today->tm_year + 1900, today->tm_mon + 1, today->tm_mday,
                  lot[i].v.entry_hour, exit_h,
                  lot[i].v.plate, lot[i].v.type,
                  lot[i].v.size, hours, fee, rerr);

    // Clear the slot
    lot[i].occupied     = 0;
    lot[i].v.plate[0]   = '\0';
    lot[i].v.type[0]    = '\0';
    lot[i].v.size       = 0;
    lot[i].v.entry_hour = 0;
    lot[i].v.exit_hour  = -1;
    lot[i].v.entry_day  = 0;
    lot[i].v.entry_month = 0;
    lot[i].v.entry_year  = 0;
    return 1;
}

int update_car(int slot_id, const char *plate, const char *type,
               int size, char *err)
{
    int i = find_slot(slot_id);
    if (i < 0)
    {
        sprintf(err, "Slot not found");
        return 0;
    }
    if (lot[i].occupied == 0)
    {
        sprintf(err, "Slot is empty");
        return 0;
    }

    // Only update plate if a new one was given and it actually changed
    if (plate != NULL && plate[0] != '\0' && strcmp(plate, lot[i].v.plate) != 0)
    {
        if (find_plate(plate) >= 0)
        {
            sprintf(err, "Plate already in use");
            return 0;
        }
        strncpy(lot[i].v.plate, plate, MAX_PLATE - 1);
    }

    if (type != NULL && type[0] != '\0')
    {
        strncpy(lot[i].v.type, type, MAX_TYPE - 1);
    }

    if (size >= 0 && size <= 2)
    {
        lot[i].v.size = size;
    }

    return 1;
}
