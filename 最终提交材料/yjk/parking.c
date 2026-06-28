#include "parking.h"
#include "fee.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int is_valid_plate(const char *plate) {
    size_t len;

    if (plate == NULL) {
        return 0;
    }

    len = strlen(plate);
    return len > 0 && len < sizeof(((VehicleRecord *)0)->plate);
}

static void copy_plate(char dest[16], const char *src) {
    strncpy(dest, src, 15);
    dest[15] = '\0';
}

static VehicleRecord *create_vehicle_record(const char *plate,
                                            SpotLocation loc,
                                            time_t entry_time) {
    VehicleRecord *rec = (VehicleRecord *)malloc(sizeof(VehicleRecord));
    if (rec == NULL) {
        fprintf(stderr, "create_vehicle_record: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    copy_plate(rec->plate, plate);
    rec->loc = loc;
    rec->entry_time = entry_time;
    return rec;
}

static void occupy_spot(ParkingSystem *ps,
                        SpotLocation loc,
                        const char *plate,
                        time_t entry_time) {
    VehicleRecord *rec = create_vehicle_record(plate, loc, entry_time);

    ps->spots[loc.floor][loc.row][loc.col].status = SPOT_OCCUPIED;
    copy_plate(ps->spots[loc.floor][loc.row][loc.col].plate, plate);
    ps->spots[loc.floor][loc.row][loc.col].entry_time = entry_time;
    ht_insert(&ps->plate_index, rec);
}

static void ensure_history_capacity(ParkingSystem *ps) {
    TransactionRecord *new_history;
    int new_capacity;

    if (ps->history_count < ps->history_capacity) {
        return;
    }

    new_capacity = ps->history_capacity > 0 ? ps->history_capacity * 2 : 100;
    new_history = (TransactionRecord *)realloc(
        ps->history, sizeof(TransactionRecord) * new_capacity);
    if (new_history == NULL) {
        fprintf(stderr, "ensure_history_capacity: history expansion failed\n");
        exit(EXIT_FAILURE);
    }

    ps->history = new_history;
    ps->history_capacity = new_capacity;
}

static void append_transaction(ParkingSystem *ps,
                               const VehicleRecord *rec,
                               time_t exit_time,
                               int duration_minutes,
                               double fee) {
    TransactionRecord *record;

    ensure_history_capacity(ps);

    record = &ps->history[ps->history_count++];
    copy_plate(record->plate, rec->plate);
    record->loc = rec->loc;
    record->entry_time = rec->entry_time;
    record->exit_time = exit_time;
    record->duration_minutes = duration_minutes;
    record->fee = fee;
}

EntryResult vehicle_entry(ParkingSystem *ps, const char *plate) {
    if (ps == NULL || !is_valid_plate(plate)) {
        return PLATE_EXISTS;
    }

    if (ht_find(&ps->plate_index, plate) != NULL || find_in_queue(ps, plate) != -1) {
        return PLATE_EXISTS;
    }

    if (stack_is_empty(&ps->free_spots)) {
        queue_enqueue(&ps->wait_queue, plate, time(NULL));
        return PARKING_FULL;
    }

    occupy_spot(ps, stack_pop_random(&ps->free_spots), plate, time(NULL));
    return ENTRY_SUCCESS;
}

ExitResult vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee) {
    VehicleRecord *rec;
    SpotLocation released_loc;
    time_t now;
    int duration_minutes;
    double fee;

    if (out_fee != NULL) {
        *out_fee = 0.0;
    }

    if (ps == NULL || !is_valid_plate(plate)) {
        return NOT_FOUND;
    }

    rec = ht_find(&ps->plate_index, plate);
    if (rec == NULL) {
        return find_in_queue(ps, plate) == -1 ? NOT_FOUND : NOT_PARKED;
    }

    released_loc = rec->loc;
    now = time(NULL);
    duration_minutes = (int)(difftime(now, rec->entry_time) / 60.0);
    if (duration_minutes < 0) {
        duration_minutes = 0;
    }

    fee = calculate_fee(&ps->fee_rule, duration_minutes);
    if (out_fee != NULL) {
        *out_fee = fee;
    }

    append_transaction(ps, rec, now, duration_minutes, fee);
    ps->total_revenue += fee;

    ps->spots[released_loc.floor][released_loc.row][released_loc.col].status = SPOT_FREE;
    ps->spots[released_loc.floor][released_loc.row][released_loc.col].plate[0] = '\0';
    ps->spots[released_loc.floor][released_loc.row][released_loc.col].entry_time = 0;

    ht_remove(&ps->plate_index, plate);
    free(rec);

    if (!queue_is_empty(&ps->wait_queue)) {
        char next_plate[16];
        time_t queue_arrival_time;

        if (queue_dequeue(&ps->wait_queue, next_plate, &queue_arrival_time)) {
            (void)queue_arrival_time;
            occupy_spot(ps, released_loc, next_plate, now);
        }
    } else {
        stack_push(&ps->free_spots, released_loc);
    }

    return PARK_EXIT_SUCCESS;
}

VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate) {
    if (ps == NULL || !is_valid_plate(plate)) {
        return NULL;
    }

    return ht_find(&ps->plate_index, plate);
}

int find_in_queue(ParkingSystem *ps, const char *plate) {
    QueueNode *p;
    int pos = 1;

    if (ps == NULL || !is_valid_plate(plate)) {
        return -1;
    }

    p = ps->wait_queue.front;
    while (p != NULL) {
        if (strcmp(p->plate, plate) == 0) {
            return pos;
        }
        p = p->next;
        pos++;
    }

    return -1;
}
