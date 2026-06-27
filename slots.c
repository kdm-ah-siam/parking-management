// slots.c

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parking.h"

struct Slot  *lot       = NULL;
int           num_slots = 0;
double        income    = 0.0;
const char   *SIZE_NAMES[3] = {"Small", "Medium", "Big"};
double        fee_rates[3]  = {1.0, 2.0, 3.0};
double        daily_cap[3]  = {8.0, 16.0, 24.0};

void init_lot(int n) {
    free(lot);
    lot = malloc(n * sizeof(struct Slot));
    if (!lot) { num_slots = 0; return; }
    memset(lot, 0, n * sizeof(struct Slot));
    num_slots = n;
    for (int i = 0; i < n; i++) {
        lot[i].id = i + 1;
        lot[i].size = 1;
        lot[i].v.exit_hour = -1;
    }
}

void add_slots_n(int n, int slot_size) {
    if (n <= 0) return;
    int total = num_slots + n;
    struct Slot *tmp = realloc(lot, total * sizeof(struct Slot));
    if (!tmp) return;
    lot = tmp;
    for (int i = num_slots; i < total; i++) {
        memset(&lot[i], 0, sizeof(struct Slot));
        lot[i].id = i + 1;
        lot[i].size = slot_size;
        lot[i].v.exit_hour = -1;
    }
    num_slots = total;
}

int find_slot(int id) {
    for (int i = 0; i < num_slots; i++) {
        if (lot[i].id == id) return i;
    }
    return -1;
}

int find_best_slot(int size) {
    for (int i = 0; i < num_slots; i++) {
        if (!lot[i].occupied && lot[i].size == size) return i;
    }
    return -1;
}

int find_plate(const char *plate) {
    char up[MAX_PLATE] = {0};
    for (int i = 0; plate[i] && i < MAX_PLATE - 1; i++) {
        up[i] = toupper(plate[i]);
    }
    for (int i = 0; i < num_slots; i++) {
        if (lot[i].occupied && strcmp(lot[i].v.plate, up) == 0) return i;
    }
    return -1;
}
