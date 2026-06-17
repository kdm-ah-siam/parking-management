// slots.c — parking lot array and lookup helpers

#include <stdlib.h>
#include <string.h>
#include "parking.h"

struct Slot  *lot       = NULL;
int           num_slots = 0;
double        income    = 0.0;
const char   *SIZE_NAMES[3] = {"Small", "Medium", "Big"};
double        fee_rates[3]  = {1.0, 2.0, 3.0};

void init_lot(int n) {
    free(lot);
    lot = calloc(n, sizeof(struct Slot));
    num_slots = n;
    for (int i = 0; i < n; i++) {
        lot[i].id = i + 1;
        lot[i].v.exit_hour = -1;
    }
}

void add_slots_n(int n) {
    if (n <= 0) return;
    int total = num_slots + n;
    lot = realloc(lot, total * sizeof(struct Slot));
    for (int i = num_slots; i < total; i++) {
        memset(&lot[i], 0, sizeof(struct Slot));
        lot[i].id = i + 1;
        lot[i].v.exit_hour = -1;
    }
    num_slots = total;
}

int find_slot(int id) {
    for (int i = 0; i < num_slots; i++)
        if (lot[i].id == id) return i;
    return -1;
}

int find_plate(const char *plate) {
    for (int i = 0; i < num_slots; i++)
        if (lot[i].occupied && strcmp(lot[i].v.plate, plate) == 0) return i;
    return -1;
}
