#ifndef FILE_IO_H
#define FILE_IO_H

#include "ds.h"

#define FILE_MAGIC    0x504B1204
#define FILE_VERSION  1

typedef struct {
    int magic;
    int version;
    int history_count;
    int free_count;
    int queue_count;
    double total_revenue;
    FeeRule fee_rule;
} FileHeader;

int save_data(ParkingSystem *ps, const char *filename);
int load_data(ParkingSystem *ps, const char *filename);
void rebuild_hash_from_spots(ParkingSystem *ps);

#endif
