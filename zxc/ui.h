#ifndef UI_H
#define UI_H

#include "ds.h"

void ui_main_menu(ParkingSystem *ps);
void ui_entry(ParkingSystem *ps);
void ui_exit(ParkingSystem *ps);
void ui_find(ParkingSystem *ps);
void ui_free_spots(ParkingSystem *ps);
void ui_wait_queue(ParkingSystem *ps);
void ui_parking_map(ParkingSystem *ps);
void ui_revenue(ParkingSystem *ps);
void ui_transactions(ParkingSystem *ps);
void ui_floor_usage(ParkingSystem *ps);
void ui_peak_hours(ParkingSystem *ps);
void ui_save(ParkingSystem *ps);
void ui_load(ParkingSystem *ps);

#endif
