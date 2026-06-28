#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "stats.h"

void stats_occupancy(ParkingSystem *ps) {
    if (ps == NULL) return;

    printf("\n");
    printf("═══════════════════════════════════════\n");
    printf("         车位占用概览\n");
    printf("═══════════════════════════════════════\n");
    printf("  楼层 | 占用 | 空闲 | 使用率\n");
    printf("  ─────┼──────┼──────┼──────\n");

    int total_used = 0, total_spots = 0;

    for (int f = 0; f < FLOORS; f++) {
        int used = 0;
        int floor_total = ROWS * COLS;
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED)
                    used++;

        printf("  %dF   | %4d | %4d | %5.1f%%\n",
               f + 1, used, floor_total - used,
               100.0 * used / floor_total);
        total_used += used;
        total_spots += floor_total;
    }

    printf("  ─────┼──────┼──────┼──────\n");
    printf("  总计 | %4d | %4d | %5.1f%%\n",
           total_used, total_spots - total_used,
           100.0 * total_used / total_spots);
    printf("═══════════════════════════════════════\n");
}

void stats_revenue(ParkingSystem *ps, int mode) {
    if (ps == NULL) return;

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    printf("\n");
    printf("═══════════════════════════════════════\n");

    if (mode == 0) {
        printf("         总收入统计\n");
        printf("═══════════════════════════════════════\n");
        printf("  累计总收入: %.2f 元\n", ps->total_revenue);
        printf("  历史交易数: %d 笔\n", ps->history_count);
    } else {
        double revenue = 0.0;
        int count = 0;

        if (mode == 1) {
            printf("         今日收入统计\n");
            printf("═══════════════════════════════════════\n");
            for (int i = 0; i < ps->history_count; i++) {
                struct tm *tm_entry = localtime(&ps->history[i].entry_time);
                if (tm_entry->tm_year == tm_now->tm_year &&
                    tm_entry->tm_mon  == tm_now->tm_mon  &&
                    tm_entry->tm_mday == tm_now->tm_mday) {
                    revenue += ps->history[i].fee;
                    count++;
                }
            }
        } else if (mode == 2) {
            printf("         本月收入统计\n");
            printf("═══════════════════════════════════════\n");
            for (int i = 0; i < ps->history_count; i++) {
                struct tm *tm_entry = localtime(&ps->history[i].entry_time);
                if (tm_entry->tm_year == tm_now->tm_year &&
                    tm_entry->tm_mon  == tm_now->tm_mon) {
                    revenue += ps->history[i].fee;
                    count++;
                }
            }
        } else if (mode == 3) {
            printf("         上周收入统计\n");
            printf("═══════════════════════════════════════\n");

            /* Calculate start of last week (previous Monday 00:00:00) */
            int days_since_monday = tm_now->tm_wday == 0 ? 6 : tm_now->tm_wday - 1;
            time_t last_monday = now - (days_since_monday + 7) * 86400;
            struct tm *tm_start = localtime(&last_monday);
            tm_start->tm_hour = 0; tm_start->tm_min = 0; tm_start->tm_sec = 0;
            time_t week_start = mktime(tm_start);

            time_t last_sunday = week_start + 7 * 86400 - 1;

            for (int i = 0; i < ps->history_count; i++) {
                if (ps->history[i].entry_time >= week_start &&
                    ps->history[i].entry_time <= last_sunday) {
                    revenue += ps->history[i].fee;
                    count++;
                }
            }

            char buf_start[32], buf_end[32];
            strftime(buf_start, sizeof(buf_start), "%Y-%m-%d", localtime(&week_start));
            strftime(buf_end,   sizeof(buf_end),   "%Y-%m-%d", localtime(&last_sunday));
            printf("  统计区间: %s ~ %s\n", buf_start, buf_end);
        }

        printf("  收入合计: %.2f 元\n", revenue);
        printf("  交易笔数: %d 笔\n", count);
    }
    printf("═══════════════════════════════════════\n");
}

void stats_transactions(ParkingSystem *ps) {
    if (ps == NULL) return;

    if (ps->history_count == 0) {
        printf("\n暂无历史交易记录。\n");
        return;
    }

    const int page_size = 20;
    int total_pages = (ps->history_count + page_size - 1) / page_size;
    int page = 0;

    while (page < total_pages) {
        int start = page * page_size;
        int end = start + page_size;
        if (end > ps->history_count) end = ps->history_count;

        printf("\n");
        printf("══════════════════════════════════════════════════════════════════\n");
        printf("  历史交易明细 (第 %d/%d 页，共 %d 条)\n",
               page + 1, total_pages, ps->history_count);
        printf("══════════════════════════════════════════════════════════════════\n");
        printf("  序号 | 车牌       | 位置         | 入场时间            | 出场时间            | 时长(分) | 费用\n");
        printf("  ─────┼────────────┼──────────────┼─────────────────────┼─────────────────────┼──────────┼───────\n");

        for (int i = start; i < end; i++) {
            TransactionRecord *tr = &ps->history[i];
            char entry_str[20], exit_str[20];
            strftime(entry_str, sizeof(entry_str), "%Y-%m-%d %H:%M:%S",
                     localtime(&tr->entry_time));
            strftime(exit_str, sizeof(exit_str), "%Y-%m-%d %H:%M:%S",
                     localtime(&tr->exit_time));

            printf("  %4d | %-10s | %dF-%d排-%d列    | %s | %s | %8d | %6.2f\n",
                   i + 1, tr->plate,
                   tr->loc.floor + 1, tr->loc.row + 1, tr->loc.col + 1,
                   entry_str, exit_str, tr->duration_minutes, tr->fee);
        }
        printf("══════════════════════════════════════════════════════════════════\n");

        page++;
        if (page < total_pages) {
            printf("按 Enter 继续下一页，按 q 退出: ");
            int ch = getchar();
            if (ch == 'q' || ch == 'Q') break;
            /* Consume remaining newline if any */
            if (ch != '\n') while (getchar() != '\n');
        }
    }
}

void stats_plate_history(ParkingSystem *ps, const char *plate) {
    if (ps == NULL || plate == NULL) return;

    printf("\n");
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("  车牌 %s 的历史记录\n", plate);
    printf("══════════════════════════════════════════════════════════════════\n");

    int found = 0;
    for (int i = 0; i < ps->history_count; i++) {
        TransactionRecord *tr = &ps->history[i];
        if (strcmp(tr->plate, plate) == 0) {
            char entry_str[20], exit_str[20];
            strftime(entry_str, sizeof(entry_str), "%Y-%m-%d %H:%M:%S",
                     localtime(&tr->entry_time));
            strftime(exit_str, sizeof(exit_str), "%Y-%m-%d %H:%M:%S",
                     localtime(&tr->exit_time));
            printf("  #%d | 位置: %dF-%d排-%d列 | 入场: %s | 出场: %s | 时长: %d分 | 费用: %.2f\n",
                   i + 1, tr->loc.floor + 1, tr->loc.row + 1, tr->loc.col + 1,
                   entry_str, exit_str, tr->duration_minutes, tr->fee);
            found++;
        }
    }

    if (found == 0)
        printf("  无记录\n");
    else
        printf("  共找到 %d 条记录\n", found);
    printf("══════════════════════════════════════════════════════════════════\n");
}

void stats_floor_usage(ParkingSystem *ps) {
    if (ps == NULL) return;

    typedef struct {
        int floor;
        int count;
        double turnover;
    } FloorStat;

    FloorStat stats[FLOORS];
    int spots_per_floor = ROWS * COLS;

    for (int f = 0; f < FLOORS; f++) {
        stats[f].floor = f;
        stats[f].count = 0;
    }

    for (int i = 0; i < ps->history_count; i++)
        stats[ps->history[i].loc.floor].count++;

    /* 加上当前在场车辆 */
    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED)
                    stats[f].count++;

    for (int f = 0; f < FLOORS; f++)
        stats[f].turnover = (double)stats[f].count / spots_per_floor;

    /* Sort by turnover descending (simple bubble — only 3 floors) */
    for (int i = 0; i < FLOORS - 1; i++)
        for (int j = i + 1; j < FLOORS; j++)
            if (stats[j].turnover > stats[i].turnover) {
                FloorStat tmp = stats[i];
                stats[i] = stats[j];
                stats[j] = tmp;
            }

    printf("\n");
    printf("═══════════════════════════════════════\n");
    printf("         楼层热度排名（按周转率）\n");
    printf("═══════════════════════════════════════\n");
    for (int i = 0; i < FLOORS; i++) {
        printf("  %d. %dF — %d次 — 周转率 %.2f\n",
               i + 1, stats[i].floor + 1, stats[i].count, stats[i].turnover);
    }
    printf("═══════════════════════════════════════\n");
}

void stats_peak_hours(ParkingSystem *ps) {
    if (ps == NULL) return;

    int hour_count[24] = {0};

    /* 历史交易（已出场车辆） */
    for (int i = 0; i < ps->history_count; i++) {
        struct tm *tm_entry = localtime(&ps->history[i].entry_time);
        hour_count[tm_entry->tm_hour]++;
    }

    /* 当前在场车辆 */
    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED) {
                    struct tm *tm_entry = localtime(&ps->spots[f][r][c].entry_time);
                    hour_count[tm_entry->tm_hour]++;
                }

    /* Find top 3 hours */
    int top_hours[3] = {-1, -1, -1};

    for (int h = 0; h < 24; h++) {
        if (hour_count[h] == 0) continue;

        /* Insert into sorted top 3 */
        for (int j = 0; j < 3; j++) {
            if (top_hours[j] == -1 || hour_count[h] > hour_count[top_hours[j]]) {
                /* Shift remaining down */
                for (int k = 2; k > j; k--)
                    top_hours[k] = top_hours[k - 1];
                top_hours[j] = h;
                break;
            }
        }
    }

    printf("\n");
    printf("═══════════════════════════════════════\n");
    printf("         峰值时段 Top 3\n");
    printf("═══════════════════════════════════════\n");
    for (int i = 0; i < 3; i++) {
        if (top_hours[i] == -1) break;
        printf("  %d. %02d:00-%02d:00 — %d 辆车入场\n",
               i + 1, top_hours[i], top_hours[i] + 1, hour_count[top_hours[i]]);
    }
    if (top_hours[0] == -1)
        printf("  暂无入场记录\n");
    printf("═══════════════════════════════════════\n");
}
