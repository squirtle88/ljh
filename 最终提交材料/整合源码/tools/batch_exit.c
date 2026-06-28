#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "parking.h"

int main(void) {
    ParkingSystem ps;
    ds_init(&ps);

    /* 加载测试数据 */
    if (!load_data(&ps, "data/parking_data.dat")) {
        printf("未找到 parking_data.dat，请先运行 gen_test_data.exe\n");
        ds_destroy(&ps);
        return 1;
    }
    rebuild_hash_from_spots(&ps);

    int total = TOTAL_SPOTS - stack_size(&ps.free_spots);
    printf("当前在场车辆: %d 辆\n\n", total);
    printf("开始批量出场结算...\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  车牌       入场时间            停留     费用\n");
    printf("────────────────────────────────────────────────────────\n");

    double total_revenue_batch = 0.0;
    int exited = 0;

    /* 遍历所有车位，逐辆出场 */
    for (int f = 0; f < FLOORS; f++) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (ps.spots[f][r][c].status == SPOT_OCCUPIED) {
                    char plate[16];
                    strcpy(plate, ps.spots[f][r][c].plate);

                    double fee = 0.0;
                    ExitResult result = vehicle_exit(&ps, plate, &fee);

                    if (result == PARK_EXIT_SUCCESS) {
                        exited++;
                        total_revenue_batch += fee;

                        /* 从 history 中取最后一条（刚生成的）看详情 */
                        TransactionRecord *tr = &ps.history[ps.history_count - 1];
                        char in_str[20], out_str[20];
                        strftime(in_str, sizeof(in_str), "%m-%d %H:%M",
                                 localtime(&tr->entry_time));
                        strftime(out_str, sizeof(out_str), "%m-%d %H:%M",
                                 localtime(&tr->exit_time));

                        printf("  %-10s %s → %s  %4d分  %7.2f元\n",
                               plate, in_str, out_str,
                               tr->duration_minutes, tr->fee);
                    } else {
                        printf("  %-10s 出场失败！\n", plate);
                    }
                }
            }
        }
    }

    printf("────────────────────────────────────────────────────────\n");
    printf("  合计: %d 辆车, 总收入: %.2f 元\n", exited, total_revenue_batch);
    printf("════════════════════════════════════════════════════════\n");

    /* 保存 */
    save_data(&ps, "data/parking_data.dat");
    printf("\n数据已保存 → 现在运行 parking.exe 即可查看完整统计\n");
    printf("  - 收入统计（选项7）\n");
    printf("  - 历史交易明细（选项8）\n");
    printf("  - 楼层热度分析（选项9）\n");
    printf("  - 峰值时段分析（选项10）\n");

    ds_destroy(&ps);
    return 0;
}
