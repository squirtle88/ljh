#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "parking.h"

/* 注入单辆车（与 gen_test_data 相同逻辑） */
static void inject(ParkingSystem *ps, const char *plate, int f, int r, int c, time_t t) {
    int found = -1;
    for (int i = 0; i <= ps->free_spots.top; i++) {
        SpotLocation loc = ps->free_spots.spots[i];
        if (loc.floor == f && loc.row == r && loc.col == c) { found = i; break; }
    }
    if (found == -1) return;
    ps->free_spots.spots[found] = ps->free_spots.spots[ps->free_spots.top];
    ps->free_spots.top--;
    ps->spots[f][r][c].status = SPOT_OCCUPIED;
    strncpy(ps->spots[f][r][c].plate, plate, 15);
    ps->spots[f][r][c].plate[15] = '\0';
    ps->spots[f][r][c].entry_time = t;
    VehicleRecord *rec = (VehicleRecord *)malloc(sizeof(VehicleRecord));
    strncpy(rec->plate, plate, 15); rec->plate[15] = '\0';
    rec->loc = make_loc(f, r, c);
    rec->entry_time = t;
    ht_insert(&ps->plate_index, rec);
}

int main(void) {
    ParkingSystem ps;
    ds_init(&ps);
    time_t now = time(NULL);

    /* ══════════ 第一批：注入15辆历史车辆 ══════════ */
    printf("═══════════════════════════════════════\n");
    printf("  第一步：注入15辆车（不同时段）\n");
    printf("═══════════════════════════════════════\n");
    inject(&ps, "京A00001", 0,0,0, now - 10*60);
    inject(&ps, "京A00002", 0,0,1, now - 20*60);
    inject(&ps, "京A00003", 0,0,2, now - 45*60);
    inject(&ps, "京A00004", 0,0,3, now - 75*60);
    inject(&ps, "京B55555", 0,2,4, now - 480*60);
    inject(&ps, "京B33333", 0,3,0, now - 30*60);
    inject(&ps, "京A00005", 1,0,0, now - 120*60);
    inject(&ps, "京A00006", 1,0,1, now - 200*60);
    inject(&ps, "京A00007", 1,0,2, now - 360*60);
    inject(&ps, "京A00008", 1,0,3, now - 500*60);
    inject(&ps, "京A00009", 2,0,0, now - 600*60);
    inject(&ps, "京A00010", 2,0,1, now - 1440*60);
    inject(&ps, "京B11111", 2,1,0, now - 15*60);
    inject(&ps, "京B22222", 2,2,0, now - 90*60);
    inject(&ps, "京C12345", 2,3,4, now - 5*60);
    printf("已注入 15 辆车\n\n");

    /* ══════════ 第二步：全部出场，生成历史记录 ══════════ */
    printf("═══════════════════════════════════════\n");
    printf("  第二步：批量出场，生成交易记录\n");
    printf("═══════════════════════════════════════\n");
    double total = 0.0;
    int count = 0;
    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps.spots[f][r][c].status == SPOT_OCCUPIED) {
                    char plate[16]; strcpy(plate, ps.spots[f][r][c].plate);
                    double fee = 0.0;
                    vehicle_exit(&ps, plate, &fee);
                    total += fee; count++;
                }
    printf("已结算 %d 笔, 收入 %.2f 元\n\n", count, total);

    /* ══════════ 第三步：再注入一批新车（供入场测试） ══════════ */
    printf("═══════════════════════════════════════\n");
    printf("  第三步：注入新车（供后续测试）\n");
    printf("═══════════════════════════════════════\n");
    inject(&ps, "沪D10001", 0,0,0, now);
    inject(&ps, "沪D10002", 0,0,1, now);
    inject(&ps, "沪D10003", 1,0,0, now);
    inject(&ps, "沪D10004", 2,0,0, now);
    inject(&ps, "沪D10005", 2,3,4, now);
    printf("已注入 5 辆新车（刚入场，免费时段内）\n\n");

    /* ══════════ 保存 ══════════ */
    save_data(&ps, "data/parking_data.dat");
    printf("═══════════════════════════════════════\n");
    printf("  数据就绪！\n");
    printf("  - 历史交易: %d 笔\n", ps.history_count);
    printf("  - 累计收入: %.2f 元\n", ps.total_revenue);
    printf("  - 当前在场: %d 辆\n", TOTAL_SPOTS - stack_size(&ps.free_spots));
    printf("═══════════════════════════════════════\n");

    ds_destroy(&ps);
    return 0;
}
