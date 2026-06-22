#ifndef PARKING_H
#define PARKING_H

#include <time.h>

#define MAX_PLATE   20
#define MAX_TYPE    20
#define DATA_FILE   "parking_data.txt"
#define REPORT_FILE "reports.txt"
#define MAX_REPORTS 1000

struct Vehicle {
    char plate[MAX_PLATE];
    char type[MAX_TYPE];
    int  size;           // 0=small  1=medium  2=big
    int  entry_hour, entry_min, entry_sec;
    int  exit_hour;
    int  entry_day, entry_month, entry_year;
};

struct Slot {
    int id;
    int occupied;
    struct Vehicle v;
};

struct Transaction {
    int    year, month, day;
    int    entry_h, exit_h;
    char   plate[MAX_PLATE];
    char   type[MAX_TYPE];
    int    size, hours;
    double fee;
};

extern struct Slot  *lot;
extern int           num_slots;
extern double        income;
extern const char   *SIZE_NAMES[3];
extern double        fee_rates[3];

// slots.c
void init_lot(int n);
void add_slots_n(int n);
int  find_slot(int id);
int  find_plate(const char *plate);

// vehicle.c
int    park_car(int slot_id, const char *plate, const char *type,
                int size, char *err);
int    remove_car(int slot_id, double *fee_out, int *hours_out, char *err);
int    update_car(int slot_id, const char *plate, const char *type,
                  int size, char *err);
double calc_fee(int size, int hours);
time_t vehicle_entry_time(const struct Vehicle *v);

// fileio.c
int save_file(char *err);
int load_file(char *err);
int append_report(int year, int month, int day,
                  int entry_h, int exit_h, const char *plate, const char *type,
                  int size, int hours, double fee, char *err);
int read_reports(int year, int month, int day,
                 struct Transaction *out, int max);
int read_reports_by_plate(const char *plate,
                          struct Transaction *out, int max);

#endif
