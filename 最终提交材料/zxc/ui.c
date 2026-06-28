#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ui.h"
#include "parking.h"
#include "fee.h"
#include "stats.h"
#include "file_io.h"

static void auto_save(ParkingSystem *ps);

/* ═══════════════════════════════════════════
   登录界面
   ═══════════════════════════════════════════ */
int ui_login(UserDatabase *db, UserRole *out_role, char out_username[32]) {
    char username[32], password[32];
    int attempts = 0;

    printf("\n");
    printf("+--------------------------------------+\n");
    printf("|     立体多层停车场管理系统           |\n");
    printf("|           登  录                     |\n");
    printf("+--------------------------------------+\n");

    while (attempts < 3) {
        printf("  用户名: ");
        scanf("%31s", username);
        getchar();

        printf("  密  码: ");
        scanf("%31s", password);
        getchar();

        UserRole role;
        if (user_db_find(db, username, password, &role)) {
            *out_role = role;
            strcpy(out_username, username);

            const char *role_name[] = {"管理员", "操作员", "车主"};
            printf("\n  登录成功！角色: %s\n", role_name[role]);
            printf("+--------------------------------------+\n");
            printf("  按回车键进入系统...");
            getchar();
            return 1;
        }

        attempts++;
        printf("  用户名或密码错误！(%d/3)\n\n", attempts);
    }

    printf("\n  登录失败次数过多，系统退出。\n");
    printf("+--------------------------------------+\n");
    return 0;
}

/* ═══════════════════════════════════════════
   管理员 — 用户管理子菜单
   ═══════════════════════════════════════════ */
void ui_user_manage(UserDatabase *db) {
    int choice;
    do {
        printf("\n");
        printf("  ┌──── 用户管理 ────┐\n");
        printf("  │  1. 添加用户     │\n");
        printf("  │  2. 删除用户     │\n");
        printf("  │  3. 用户列表     │\n");
        printf("  │  0. 返回         │\n");
        printf("  └──────────────────┘\n");
        printf("  请选择: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1: {
                char name[32], pass[32], plate[16] = "";
                int role_choice;
                printf("  角色: 1.管理员 2.操作员 3.车主\n");
                printf("  选择: ");
                scanf("%d", &role_choice);
                getchar();
                if (role_choice < 1 || role_choice > 3) {
                    printf("  无效角色！\n");
                    break;
                }
                if (role_choice == 3) {
                    printf("  绑定车牌号: ");
                    scanf("%15s", plate);
                    getchar();
                }
                printf("  输入新用户名: ");
                scanf("%31s", name);
                getchar();
                if (user_db_exists(db, name)) {
                    printf("  用户名已存在！\n");
                    break;
                }
                printf("  输入密码: ");
                scanf("%31s", pass);
                getchar();
                UserRole roles[] = {USER_ADMIN, USER_OPERATOR, USER_CUSTOMER};
                if (user_db_add(db, name, pass, roles[role_choice - 1], plate))
                    printf("  用户 %s 添加成功！\n", name);
                else
                    printf("  添加失败！\n");
                break;
            }
            case 2: {
                char name[32];
                printf("  输入要删除的用户名: ");
                scanf("%31s", name);
                getchar();
                if (user_db_delete(db, name))
                    printf("  用户 %s 已删除。\n", name);
                else
                    printf("  删除失败（用户不存在或是唯一管理员）！\n");
                break;
            }
            case 3: {
                printf("\n");
                printf("  ═══════════════════════════════════════\n");
                printf("           当前用户列表 (%d人)\n", db->count);
                printf("  ═══════════════════════════════════════\n");
                printf("  用户名         | 角色\n");
                printf("  ───────────────┼──────────\n");
                const char *rnames[] = {"管理员", "操作员", "车主"};
                for (int i = 0; i < db->count; i++) {
                    printf("  %-15s | %s\n",
                           db->users[i].username,
                           rnames[db->users[i].role]);
                }
                printf("  ═══════════════════════════════════════\n");
                break;
            }
            case 0: break;
            default: printf("  无效选择！\n");
        }
    } while (choice != 0);
}

/* ═══════════════════════════════════════════
   管理员菜单（全部功能 + 用户管理）
   ═══════════════════════════════════════════ */
void ui_admin_menu(ParkingSystem *ps) {
    int choice;
    do {
        printf("\n");
        printf("+--------------------------------------+\n");
        printf("|   立体多层停车场管理系统 [管理员]    |\n");
        printf("+--------------------------------------+\n");
        printf("|  1. 车辆入场登记                     |\n");
        printf("|  2. 车辆出场结算                     |\n");
        printf("|  3. 查找车辆                         |\n");
        printf("|  4. 空闲车位查看                     |\n");
        printf("|  5. 等待队列查看                     |\n");
        printf("|  6. 车位状态总览 (ASCII图)           |\n");
        printf("|  7. 收入统计                         |\n");
        printf("|  8. 历史交易明细                     |\n");
        printf("|  9. 楼层热度分析                     |\n");
        printf("| 10. 峰值时段分析                     |\n");
        printf("| 11. 保存数据                         |\n");
        printf("| 12. 加载数据                         |\n");
        printf("| 13. 用户管理                         |\n");
        printf("|  0. 退出系统                         |\n");
        printf("+--------------------------------------+\n");
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
            case 13: ui_user_manage(&ps->user_db); break;
            case 0:
                printf("感谢使用，正在保存数据...\n");
                break;
            default:
                printf("无效选择，请重新输入！\n");
        }
        if (choice != 0) auto_save(ps);
    } while (choice != 0);
}

/* ═══════════════════════════════════════════
   操作员菜单（入场/出场/查询/查看）
   ═══════════════════════════════════════════ */
void ui_operator_menu(ParkingSystem *ps) {
    int choice;
    do {
        printf("\n");
        printf("+--------------------------------------+\n");
        printf("|   立体多层停车场管理系统 [操作员]    |\n");
        printf("+--------------------------------------+\n");
        printf("|  1. 车辆入场登记                     |\n");
        printf("|  2. 车辆出场结算                     |\n");
        printf("|  3. 查找车辆                         |\n");
        printf("|  4. 空闲车位查看                     |\n");
        printf("|  5. 等待队列查看                     |\n");
        printf("|  6. 车位状态总览 (ASCII图)           |\n");
        printf("|  7. 保存数据                         |\n");
        printf("|  8. 加载数据                         |\n");
        printf("|  0. 退出系统                         |\n");
        printf("+--------------------------------------+\n");
        printf("请选择操作: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1: ui_entry(ps);       break;
            case 2: ui_exit(ps);        break;
            case 3: ui_find(ps);        break;
            case 4: ui_free_spots(ps);  break;
            case 5: ui_wait_queue(ps);  break;
            case 6: ui_parking_map(ps); break;
            case 7: ui_save(ps);        break;
            case 8: ui_load(ps);        break;
            case 0:
                printf("感谢使用，正在保存数据...\n");
                break;
            default:
                printf("无效选择，请重新输入！\n");
        }
        if (choice != 0) auto_save(ps);
    } while (choice != 0);
}

/* ═══════════════════════════════════════════
   车主菜单（仅查询：自动查自己车/看空位/看地图）
   ═══════════════════════════════════════════ */
void ui_customer_menu(ParkingSystem *ps, const char *my_plate) {
    int choice;
    do {
        printf("\n");
        printf("+--------------------------------------+\n");
        printf("|   立体多层停车场管理系统 [车主]      |\n");
        printf("|   我的车牌: %-24s |\n", my_plate);
        printf("+--------------------------------------+\n");
        printf("|  1. 查找我的车辆                     |\n");
        printf("|  2. 空闲车位查看                     |\n");
        printf("|  3. 车位状态总览 (ASCII图)           |\n");
        printf("|  0. 退出系统                         |\n");
        printf("+--------------------------------------+\n");
        printf("请选择操作: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1: {
                VehicleRecord *rec = find_vehicle(ps, my_plate);
                if (rec) {
                    char ts[64];
                    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&rec->entry_time));
                    int dur = (int)(difftime(time(NULL), rec->entry_time) / 60.0);
                    if (dur < 0) dur = 0;
                    double fee = calculate_fee(&ps->fee_rule, dur);
                    printf("车辆 %s 在场内\n位置：%dF-%d排-%d列\n入场时间：%s\n",
                           my_plate, rec->loc.floor+1, rec->loc.row+1, rec->loc.col+1, ts);
                    printf("已停时长：%d 分钟  |  当前费用：%.2f 元\n", dur, fee);
                } else {
                    int qp = find_in_queue(ps, my_plate);
                    if (qp != -1)
                        printf("车辆 %s 在等待队列中，排第 %d 位\n", my_plate, qp);
                    else
                        printf("车辆 %s 不在场内，可能尚未入场或已出场。\n", my_plate);
                }
                break;
            }
            case 2: ui_free_spots(ps);  break;
            case 3: ui_parking_map(ps); break;
            case 0:
                printf("感谢使用，再见！\n");
                break;
            default:
                printf("无效选择，请重新输入！\n");
        }
        if (choice != 0) auto_save(ps);
    } while (choice != 0);
}

/* ═══════════════════════════════════════════
   各功能界面（保持不变）
   ═══════════════════════════════════════════ */

void ui_entry(ParkingSystem *ps) {
    char plate[16];
    printf("请输入车牌号: ");
    scanf("%15s", plate);
    getchar();

    EntryResult result = vehicle_entry(ps, plate);
    switch (result) {
        case ENTRY_SUCCESS:
            printf("车辆 %s 入场登记成功！\n", plate);
            break;
        case PARKING_FULL:
            printf("车位已满，车辆 %s 已加入等待队列。\n", plate);
            break;
        case PLATE_EXISTS:
            printf("车牌 %s 已存在（可能已入场或在队列中），无法重复登记！\n", plate);
            break;
    }
}

void ui_exit(ParkingSystem *ps) {
    char plate[16];
    double fee = 0.0;
    printf("请输入车牌号: ");
    scanf("%15s", plate);
    getchar();

    ExitResult result = vehicle_exit(ps, plate, &fee);
    switch (result) {
        case PARK_EXIT_SUCCESS:
            printf("车辆 %s 出场成功，费用: %.2f 元\n", plate, fee);
            break;
        case NOT_FOUND:
            printf("未找到车辆 %s，可能车牌输入有误或车辆不在场内。\n", plate);
            break;
        case NOT_PARKED:
            printf("车辆 %s 在等待队列中，尚未入场，无法结算。\n", plate);
            break;
    }
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
    save_data(ps, "data/parking_data.dat");
    save_users(&ps->user_db, "data/users.dat");
    printf("数据已保存（停车场 + %d个用户账号）\n", ps->user_db.count);
}

/* 自动保存：每次操作后调用 */
static void auto_save(ParkingSystem *ps) {
    save_data(ps, "data/parking_data.dat");
    save_users(&ps->user_db, "data/users.dat");
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

    if (load_data(ps, "data/parking_data.dat")) {
        rebuild_hash_from_spots(ps);
    } else {
        printf("未找到数据文件，使用初始状态。\n");
    }
}
