#ifndef PARKING_H
#define PARKING_H

#define MAX_PLATE  20
#define MAX_TYPE   20
#define DATA_FILE  "parking_data.txt"

struct Vehicle {
    char plate[MAX_PLATE];
    char type[MAX_TYPE];
    int  size;          // 0=small  1=medium  2=big
    int  entry_hour;
    int  exit_hour;     // -1 = still parked
};

struct Slot {
    int id;
    int occupied;       // 1=taken  0=free
    struct Vehicle v;
};

extern struct Slot    *lot;
extern int             num_slots;
extern double          income;
extern const char     *SIZE_NAMES[3];

// slots.c
void init_lot(int n);
void add_slots_n(int n);
int  find_slot(int id);
int  find_plate(const char *plate);

// vehicle.c
int    park_car(int slot_id, const char *plate, const char *type,
                int size, int entry, char *err);
int    remove_car(int slot_id, int exit_h,
                  double *fee_out, int *hours_out, char *err);
int    update_car(int slot_id, const char *plate, const char *type,
                  int size, char *err);
double calc_fee(int size, int hours);

// fileio.c
int save_file(char *err);
int load_file(char *err);

#endif
