#ifndef PARKING_H
#define PARKING_H

#include "../pzx/ds.h"

EntryResult vehicle_entry(ParkingSystem *ps, const char *plate);
ExitResult vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee);
VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate);
int find_in_queue(ParkingSystem *ps, const char *plate);

#endif
