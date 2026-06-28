#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "parking.h"

static void inject_vehicle(ParkingSystem *ps, const char *plate, int f, int r, int c, time_t entry_time) {
    int found = -1;
    for (int i = 0; i <= ps->free_spots.top; i++) {
        SpotLocation loc = ps->free_spots.spots[i];
        if (loc.floor == f && loc.row == r && loc.col == c) { found = i; break; }
    }
    if (found == -1) { printf("位置占用!\n"); return; }
    ps->free_spots.spots[found] = ps->free_spots.spots[ps->free_spots.top];
    ps->free_spots.top--;
    ps->spots[f][r][c].status = SPOT_OCCUPIED;
    strncpy(ps->spots[f][r][c].plate, plate, 15);
    ps->spots[f][r][c].plate[15] = '\0';
    ps->spots[f][r][c].entry_time = entry_time;
    VehicleRecord *rec = (VehicleRecord *)malloc(sizeof(VehicleRecord));
    strncpy(rec->plate, plate, 15); rec->plate[15] = '\0';
    rec->loc = make_loc(f, r, c);
    rec->entry_time = entry_time;
    ht_insert(&ps->plate_index, rec);
    printf("  %s @ %dF-%d-%d ", plate, f+1, r+1, c+1);
    char buf[64]; strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&entry_time));
    printf("%s\n", buf);
}

int main(void) {
    ParkingSystem ps;
    ds_init(&ps);
    time_t now = time(NULL);

    printf("当前时间: %s", ctime(&now));
    printf("注入15辆测试车辆...\n");

    inject_vehicle(&ps, "京A00001", 0, 0, 0, now - 10*60);
    inject_vehicle(&ps, "京A00002", 0, 0, 1, now - 20*60);
    inject_vehicle(&ps, "京A00003", 0, 0, 2, now - 45*60);
    inject_vehicle(&ps, "京A00004", 0, 0, 3, now - 75*60);
    inject_vehicle(&ps, "京A00005", 1, 0, 0, now - 120*60);
    inject_vehicle(&ps, "京A00006", 1, 0, 1, now - 200*60);
    inject_vehicle(&ps, "京A00007", 1, 0, 2, now - 360*60);
    inject_vehicle(&ps, "京A00008", 1, 0, 3, now - 500*60);
    inject_vehicle(&ps, "京A00009", 2, 0, 0, now - 600*60);
    inject_vehicle(&ps, "京A00010", 2, 0, 1, now - 1440*60);
    inject_vehicle(&ps, "京B11111", 2, 1, 0, now - 15*60);
    inject_vehicle(&ps, "京B22222", 2, 2, 0, now - 90*60);
    inject_vehicle(&ps, "京B33333", 0, 3, 0, now - 30*60);
    inject_vehicle(&ps, "京B55555", 0, 2, 4, now - 480*60);
    inject_vehicle(&ps, "京C12345", 2, 3, 4, now - 5*60);

    printf("\n占用: %d, 空闲: %d\n", TOTAL_SPOTS - stack_size(&ps.free_spots), stack_size(&ps.free_spots));
    save_data(&ps, "data/parking_data.dat");
    printf("测试数据已保存到 data/parking_data.dat\n");
    ds_destroy(&ps);
    return 0;
}
