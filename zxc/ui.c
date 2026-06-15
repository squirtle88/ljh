#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ui.h"
#include "parking.h"
#include "stats.h"
#include "file_io.h"

void ui_main_menu(ParkingSystem *ps) {
    int choice;
    do {
        printf("\n");
        printf("╔══════════════════════════════════════╗\n");
        printf("║       立体多层停车场管理系统          ║\n");
        printf("╠══════════════════════════════════════╣\n");
        printf("║  1. 车辆入场登记                     ║\n");
        printf("║  2. 车辆出场结算                     ║\n");
        printf("║  3. 查找车辆                         ║\n");
        printf("║  4. 空闲车位查看                     ║\n");
        printf("║  5. 等待队列查看                     ║\n");
        printf("║  6. 车位状态总览 (ASCII图)            ║\n");
        printf("║  7. 收入统计                         ║\n");
        printf("║  8. 历史交易明细                     ║\n");
        printf("║  9. 楼层热度分析                     ║\n");
        printf("║ 10. 峰值时段分析                     ║\n");
        printf("║ 11. 保存数据                         ║\n");
        printf("║ 12. 加载数据                         ║\n");
        printf("║  0. 退出系统                         ║\n");
        printf("╚══════════════════════════════════════╝\n");
        printf("请选择操作: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1:  ui_entry(ps);          break;
            case 2:  ui_exit(ps);           break;
            case 3:  ui_find(ps);           break;
            case 4:  ui_free_spots(ps);     break;
            case 5:  ui_wait_queue(ps);     break;
            case 6:  ui_parking_map(ps);    break;
            case 7:  ui_revenue(ps);        break;
            case 8:  ui_transactions(ps);   break;
            case 9:  ui_floor_usage(ps);    break;
            case 10: ui_peak_hours(ps);     break;
            case 11: ui_save(ps);           break;
            case 12: ui_load(ps);           break;
            case 0:
                printf("感谢使用，正在保存数据...\n");
                break;
            default:
                printf("无效选择，请重新输入！\n");
        }
    } while (choice != 0);
}

void ui_entry(ParkingSystem *ps) {
    char plate[16];
    printf("请输入车牌号: ");
    scanf("%15s", plate);
    getchar();
    vehicle_entry(ps, plate);
}

void ui_exit(ParkingSystem *ps) {
    char plate[16];
    double fee;
    printf("请输入车牌号: ");
    scanf("%15s", plate);
    getchar();
    vehicle_exit(ps, plate, &fee);
}

void ui_find(ParkingSystem *ps) {
    char plate[16];
    printf("请输入车牌号: ");
    scanf("%15s", plate);
    getchar();

    VehicleRecord *rec = find_vehicle(ps, plate);
    if (rec != NULL) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                 localtime(&rec->entry_time));
        printf("车辆 %s 在场内\n", plate);
        printf("位置：%dF-%d排-%d列\n",
               rec->loc.floor + 1, rec->loc.row + 1, rec->loc.col + 1);
        printf("入场时间：%s\n", time_str);
        return;
    }

    int qpos = find_in_queue(ps, plate);
    if (qpos != -1) {
        printf("车辆 %s 在等待队列中，排在第 %d 位\n", plate, qpos);
        return;
    }

    printf("未找到车辆 %s (可能已出场或不存在)\n", plate);
}

void ui_free_spots(ParkingSystem *ps) {
    if (ps == NULL) return;

    int free_cnt = stack_size(&ps->free_spots);

    printf("\n");
    printf("═══════════════════════════════════════\n");
    printf("         空闲车位列表\n");
    printf("═══════════════════════════════════════\n");
    printf("  空闲车位总数: %d / %d\n\n", free_cnt, TOTAL_SPOTS);

    if (free_cnt == 0) {
        printf("  当前没有空闲车位。\n");
        printf("═══════════════════════════════════════\n");
        return;
    }

    /* Group free spots by floor for organized display */
    for (int f = 0; f < FLOORS; f++) {
        int floor_free = 0;
        printf("  %dF 空闲: ", f + 1);
        int first = 1;
        for (int i = 0; i <= ps->free_spots.top; i++) {
            SpotLocation loc = ps->free_spots.spots[i];
            if (loc.floor == f) {
                if (!first) printf(", ");
                printf("%d排%d列", loc.row + 1, loc.col + 1);
                first = 0;
                floor_free++;
            }
        }
        if (floor_free == 0)
            printf("无");
        printf("\n");
    }
    printf("═══════════════════════════════════════\n");
}

void ui_wait_queue(ParkingSystem *ps) {
    if (ps == NULL) return;

    printf("\n");
    printf("═══════════════════════════════════════\n");
    printf("         等待队列\n");
    printf("═══════════════════════════════════════\n");
    printf("  排队车辆数: %d\n\n", ps->wait_queue.count);

    if (ps->wait_queue.count == 0) {
        printf("  当前没有车辆在排队。\n");
        printf("═══════════════════════════════════════\n");
        return;
    }

    printf("  序号 | 车牌       | 到达时间\n");
    printf("  ─────┼────────────┼─────────────────────\n");

    QueueNode *p = ps->wait_queue.front;
    int pos = 1;
    while (p != NULL) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                 localtime(&p->arrival_time));
        printf("  %4d | %-10s | %s\n", pos, p->plate, time_str);
        p = p->next;
        pos++;
    }
    printf("═══════════════════════════════════════\n");
}

void ui_parking_map(ParkingSystem *ps) {
    if (ps == NULL) return;

    printf("\n═══════════════════════════════════\n");
    printf("        立体停车场 — 车位状态图\n");
    printf("═══════════════════════════════════\n");
    printf("  ● 已占用   ○ 空闲\n\n");

    for (int f = FLOORS - 1; f >= 0; f--) {
        printf("─── %dF ", f + 1);
        if (f == FLOORS - 1) printf("(顶层)");
        printf(" ───\n");

        for (int r = 0; r < ROWS; r++) {
            printf("    ");
            for (int c = 0; c < COLS; c++) {
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED)
                    printf("[●]");
                else
                    printf("[○]");
            }
            printf("\n");
        }
        int floor_used = 0;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED) floor_used++;
        printf("    使用率: %d/%d (%.0f%%)\n\n",
               floor_used, ROWS * COLS, 100.0 * floor_used / (ROWS * COLS));
    }

    printf("═══════════════════════════════════\n");
    int free_cnt = stack_size(&ps->free_spots);
    printf(" 空闲车位: %d / %d  |  排队车辆: %d\n",
           free_cnt, TOTAL_SPOTS, ps->wait_queue.count);
    printf("═══════════════════════════════════\n");
}

void ui_revenue(ParkingSystem *ps) {
    if (ps == NULL) return;

    int choice;
    do {
        printf("\n");
        printf("  ┌──── 收入统计 ────┐\n");
        printf("  │  1. 总收入       │\n");
        printf("  │  2. 今日收入     │\n");
        printf("  │  3. 本月收入     │\n");
        printf("  │  4. 上周收入     │\n");
        printf("  │  0. 返回         │\n");
        printf("  └──────────────────┘\n");
        printf("  请选择: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1: stats_revenue(ps, 0); break;
            case 2: stats_revenue(ps, 1); break;
            case 3: stats_revenue(ps, 2); break;
            case 4: stats_revenue(ps, 3); break;
            case 0: break;
            default: printf("无效选择！\n");
        }
    } while (choice != 0);
}

void ui_transactions(ParkingSystem *ps) {
    if (ps == NULL) return;
    stats_transactions(ps);
}

void ui_floor_usage(ParkingSystem *ps) {
    if (ps == NULL) return;
    stats_floor_usage(ps);
}

void ui_peak_hours(ParkingSystem *ps) {
    if (ps == NULL) return;
    stats_peak_hours(ps);
}

void ui_save(ParkingSystem *ps) {
    if (ps == NULL) return;
    save_data(ps, "parking_data.dat");
}

void ui_load(ParkingSystem *ps) {
    if (ps == NULL) return;

    /* Free existing dynamic resources before loading over them */
    if (ps->history != NULL) {
        free(ps->history);
        ps->history = NULL;
    }
    if (ps->free_spots.spots != NULL) {
        free(ps->free_spots.spots);
        ps->free_spots.spots = NULL;
    }

    /* Free wait queue nodes */
    while (!queue_is_empty(&ps->wait_queue)) {
        char tmp_plate[16];
        time_t tmp_t;
        queue_dequeue(&ps->wait_queue, tmp_plate, &tmp_t);
    }

    /* Clear hash table */
    ht_clear(&ps->plate_index);

    if (load_data(ps, "parking_data.dat")) {
        rebuild_hash_from_spots(ps);
    } else {
        printf("未找到数据文件，使用初始状态。\n");
    }
}
