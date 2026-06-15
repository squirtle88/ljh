#ifndef STATS_H
#define STATS_H

#include "ds.h"

void stats_occupancy(ParkingSystem *ps);
void stats_revenue(ParkingSystem *ps, int mode);
void stats_transactions(ParkingSystem *ps);
void stats_plate_history(ParkingSystem *ps, const char *plate);
void stats_floor_usage(ParkingSystem *ps);
void stats_peak_hours(ParkingSystem *ps);

#endif
