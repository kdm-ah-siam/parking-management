#ifndef PARKING_H
#define PARKING_H

// Maximum lengths for text fields stored in each vehicle
#define MAX_PLATE    20
#define MAX_TYPE     20

// File names used for saving data
#define DATA_FILE    "parking_data.txt"
#define REPORT_FILE  "reports.txt"

// Maximum number of transaction records we can load into memory at once
#define MAX_REPORTS  1000

// Stores information about one parked vehicle
struct Vehicle {
    char plate[MAX_PLATE];   // e.g. "ABC-123"
    char type[MAX_TYPE];     // e.g. "Car", "Truck"
    int  size;               // 0 = small, 1 = medium, 2 = big
    int  entry_hour;         // hour the car arrived (0-23)
    int  exit_hour;          // hour the car left (-1 means still parked)
    int  entry_day;          // day of month when car arrived
    int  entry_month;        // month when car arrived
    int  entry_year;         // year when car arrived
};

// Stores one parking slot (either empty or holding a vehicle)
struct Slot {
    int id;                  // slot number, starting from 1
    int occupied;            // 1 = a vehicle is here, 0 = empty
    struct Vehicle v;        // the vehicle (only valid when occupied == 1)
};

// Stores one completed parking transaction (for the reports screen)
struct Transaction {
    int    year;
    int    month;
    int    day;
    int    entry_h;
    int    exit_h;
    char   plate[MAX_PLATE];
    char   type[MAX_TYPE];
    int    size;
    int    hours;
    double fee;
};

// Global variables shared across all source files
extern struct Slot  *lot;           // the array of all parking slots
extern int           num_slots;     // how many slots exist right now
extern double        income;        // total money collected so far
extern const char   *SIZE_NAMES[3]; // "Small", "Medium", "Big"
extern double        fee_rates[3];  // price per hour: [0]=small [1]=medium [2]=big

// Functions in slots.c
void init_lot(int number_of_slots);
void add_slots_n(int how_many_to_add);
int  find_slot(int slot_id);
int  find_plate(const char *plate);

// Functions in vehicle.c
int    park_car(int slot_id, const char *plate, const char *type,
                int size, int entry_hour, char *error_message);
int    remove_car(int slot_id, int exit_hour,
                  double *fee_out, int *hours_out, char *error_message);
int    update_car(int slot_id, const char *new_plate, const char *new_type,
                  int new_size, char *error_message);
double calc_fee(int size, int hours);

// Functions in fileio.c
int save_file(char *error_message);
int load_file(char *error_message);
int append_report(int year, int month, int day,
                  int entry_hour, int exit_hour,
                  const char *plate, const char *type,
                  int size, int hours, double fee, char *error_message);
int read_reports(int year, int month, int day,
                 struct Transaction *results, int max_results);
int read_reports_by_plate(const char *plate,
                          struct Transaction *results, int max_results);

#endif
