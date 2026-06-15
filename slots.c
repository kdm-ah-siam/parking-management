// slots.c - stores the parking lot data and provides lookup helpers

#include <stdlib.h>
#include <string.h>
#include "parking.h"

// Global variables (shared with all other files via parking.h)
struct Slot  *lot       = NULL;
int           num_slots = 0;
double        income    = 0.0;

const char *SIZE_NAMES[3] = { "Small", "Medium", "Big" };
double      fee_rates[3]  = { 1.0, 2.0, 3.0 };   // dollars per hour for each size


// Create a brand-new parking lot with the given number of slots.
// Any existing lot is freed first.
void init_lot(int number_of_slots)
{
    free(lot);

    lot       = calloc(number_of_slots, sizeof(struct Slot));
    num_slots = number_of_slots;

    for (int i = 0; i < number_of_slots; i++)
    {
        lot[i].id           = i + 1;   // slots numbered starting from 1
        lot[i].occupied     = 0;
        lot[i].v.exit_hour  = -1;      // -1 means "no exit time yet"
    }
}


// Add more slots to the existing parking lot.
void add_slots_n(int how_many_to_add)
{
    if (how_many_to_add <= 0)
    {
        return;
    }

    int new_total = num_slots + how_many_to_add;

    lot = realloc(lot, new_total * sizeof(struct Slot));

    // Initialize only the newly added slots
    for (int i = num_slots; i < new_total; i++)
    {
        lot[i].id            = i + 1;
        lot[i].occupied      = 0;
        lot[i].v.plate[0]    = '\0';
        lot[i].v.type[0]     = '\0';
        lot[i].v.size        = 0;
        lot[i].v.entry_hour  = 0;
        lot[i].v.exit_hour   = -1;
        lot[i].v.entry_day   = 0;
        lot[i].v.entry_month = 0;
        lot[i].v.entry_year  = 0;
    }

    num_slots = new_total;
}


// Search for a slot by its ID number.
// Returns the index in the lot[] array, or -1 if not found.
int find_slot(int slot_id)
{
    for (int i = 0; i < num_slots; i++)
    {
        if (lot[i].id == slot_id)
        {
            return i;
        }
    }
    return -1;
}


// Search for a vehicle by its plate number.
// Returns the index in the lot[] array, or -1 if not found.
int find_plate(const char *plate)
{
    for (int i = 0; i < num_slots; i++)
    {
        if (lot[i].occupied == 1)
        {
            if (strcmp(lot[i].v.plate, plate) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}
