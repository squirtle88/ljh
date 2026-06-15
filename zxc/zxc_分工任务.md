# zxc — 统计与交互层 分工任务

> 提取自：停车场管理系统_三人分工方案  
> 课程：数据结构实践 | 语言：C 语言

---

## 一、负责文件

| 文件 | 说明 |
|------|------|
| `stats.h` / `stats.c` | 统计模块声明与实现 |
| `ui.h` / `ui.c` | 菜单界面声明与实现 |
| `file_io.h` / `file_io.c` | 数据持久化（二进制文件读写） |
| `main.c` | 程序入口 |

---

## 二、统计模块 (stats.c)

### 2.1 车位占用概览

```c
void stats_occupancy(ParkingSystem *ps) {
    // 遍历 spots[FLOORS][ROWS][COLS]
    // 统计每层：占用数 / 总数
    // 输出格式示例：
    //   楼层 | 占用 | 空闲 | 使用率
    //   ─────┼──────┼──────┼──────
    //   1F   | 15   | 5    | 75.0%
    //   2F   | 10   | 10   | 50.0%
    //   3F   | 8    | 12   | 40.0%
    //   ─────┼──────┼──────┼──────
    //   总计 | 33   | 27   | 55.0%
}
```

### 2.2 收入统计

```c
void stats_revenue(ParkingSystem *ps, int mode) {
    // mode 0: 总收入 — 直接打印 ps->total_revenue
    // mode 1: 今日收入 — 遍历 history，比较 entry_time 的日期
    // mode 2: 本月收入 — 遍历 history，比较年+月
    // mode 3: 上周收入 — 遍历 history，判断是否在上一周范围内
    //
    // 时间判断使用 localtime(&tr->entry_time) → struct tm
    // 比较 tm_year, tm_mon, tm_mday
}
```

### 2.3 历史交易明细

```c
void stats_transactions(ParkingSystem *ps) {
    // 遍历 ps->history[0..history_count-1]
    // 输出每笔：车牌、位置、入场时间、出场时间、时长、费用
    // 分页显示（每页20条），按 q 退出
}
```

### 2.4 指定车牌历史

```c
void stats_plate_history(ParkingSystem *ps, const char *plate) {
    // 遍历 history，strcmp 匹配车牌
    // 输出该车的所有历史记录
    // 如果没找到 → 提示"无记录"
}
```

### 2.5 楼层热度分析

```c
void stats_floor_usage(ParkingSystem *ps) {
    // 遍历 history 数组，统计每层出现次数
    // 计算每层周转率：交易次数 / 该层车位数
    //
    // 输出示例：
    //   楼层热度排名（按周转率）：
    //   1. 1F — 120次 — 周转率 6.0
    //   2. 2F — 85次  — 周转率 4.25
    //   3. 3F — 42次  — 周转率 2.1
}
```

### 2.6 峰值时段分析

```c
void stats_peak_hours(ParkingSystem *ps) {
    // int hour_count[24] = {0};  // 每个时段的入场次数
    // 遍历 history，对每个 entry_time 提取 tm->tm_hour
    // hour_count[tm->tm_hour]++
    //
    // 输出示例：
    //   峰值时段 Top 3：
    //   1. 10:00-11:00 — 32 辆车入场
    //   2. 14:00-15:00 — 28 辆车入场
    //   3. 18:00-19:00 — 25 辆车入场
}
```

---

## 三、菜单界面 (ui.c)

### 3.1 主菜单

```c
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
        getchar();  // 清空缓冲区中的换行符

        switch (choice) {
            case 1: ui_entry(ps);           break;
            case 2: ui_exit(ps);            break;
            case 3: ui_find(ps);            break;
            case 4: ui_free_spots(ps);      break;
            case 5: ui_wait_queue(ps);      break;
            case 6: ui_parking_map(ps);     break;
            case 7: ui_revenue(ps);         break;
            case 8: ui_transactions(ps);    break;
            case 9: ui_floor_usage(ps);     break;
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
```

### 3.2 入场交互

```c
void ui_entry(ParkingSystem *ps) {
    char plate[16];
    printf("请输入车牌号: ");
    scanf("%s", plate);
    getchar();
    vehicle_entry(ps, plate);
}
```

### 3.3 出场交互

```c
void ui_exit(ParkingSystem *ps) {
    char plate[16];
    double fee;
    printf("请输入车牌号: ");
    scanf("%s", plate);
    getchar();
    vehicle_exit(ps, plate, &fee);
}
```

### 3.4 查找车辆

```c
void ui_find(ParkingSystem *ps) {
    char plate[16];
    printf("请输入车牌号: ");
    scanf("%s", plate);
    getchar();

    VehicleRecord *rec = find_vehicle(ps, plate);
    if (rec != NULL) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                 localtime(&rec->entry_time));
        printf("车辆 %s 在场内\n", plate);
        printf("位置：%dF-%d排-%d列\n",
               rec->loc.floor+1, rec->loc.row+1, rec->loc.col+1);
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
```

### 3.5 车位状态图 (ASCII)

```c
void ui_parking_map(ParkingSystem *ps) {
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
               floor_used, ROWS*COLS, 100.0*floor_used/(ROWS*COLS));
    }

    printf("═══════════════════════════════════\n");
    int free_cnt = stack_size(&ps->free_spots);
    printf(" 空闲车位: %d / %d  |  排队车辆: %d\n",
           free_cnt, TOTAL_SPOTS, ps->wait_queue.count);
    printf("═══════════════════════════════════\n");
}
```

### 3.6 其他 UI 函数（需自行实现）

```c
void ui_free_spots(ParkingSystem *ps);    // 显示所有空闲车位列表
void ui_wait_queue(ParkingSystem *ps);    // 显示等待队列详情
void ui_revenue(ParkingSystem *ps);       // 收入统计子菜单（调用 stats_revenue）
void ui_transactions(ParkingSystem *ps);  // 历史交易（调用 stats_transactions）
void ui_floor_usage(ParkingSystem *ps);   // 楼层热度（调用 stats_floor_usage）
void ui_peak_hours(ParkingSystem *ps);    // 峰值时段（调用 stats_peak_hours）
void ui_save(ParkingSystem *ps);          // 保存数据（调用 save_data）
void ui_load(ParkingSystem *ps);          // 加载数据（调用 load_data）
```

---

## 四、数据持久化 (file_io.c)

### 4.1 文件格式定义

```c
// 文件魔数，用于校验文件类型
#define FILE_MAGIC 0x504B1204
#define FILE_VERSION 1

typedef struct {
    int magic;           // FILE_MAGIC
    int version;         // FILE_VERSION
    int history_count;   // 历史记录数
    int free_count;      // 空闲车位数（用于恢复栈）
    int queue_count;     // 队列长度
    double total_revenue;
    FeeRule fee_rule;
} FileHeader;
```

### 4.2 保存数据

```c
int save_data(ParkingSystem *ps, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("无法打开文件 %s 写入\n", filename);
        return 0;
    }

    // 1. 写文件头
    FileHeader header;
    header.magic = FILE_MAGIC;
    header.version = FILE_VERSION;
    header.history_count = ps->history_count;
    header.free_count = ps->free_spots.top + 1;
    header.queue_count = ps->wait_queue.count;
    header.total_revenue = ps->total_revenue;
    header.fee_rule = ps->fee_rule;
    fwrite(&header, sizeof(FileHeader), 1, fp);

    // 2. 写所有车位状态（spots 三维数组）
    fwrite(ps->spots, sizeof(ParkingSpot), FLOORS * ROWS * COLS, fp);

    // 3. 写空闲栈内容（所有坐标）
    fwrite(ps->free_spots.spots, sizeof(SpotLocation),
           ps->free_spots.top + 1, fp);

    // 4. 写等待队列
    QueueNode *p = ps->wait_queue.front;
    while (p != NULL) {
        fwrite(p, sizeof(QueueNode), 1, fp);
        p = p->next;
    }

    // 5. 写历史交易
    fwrite(ps->history, sizeof(TransactionRecord),
           ps->history_count, fp);

    fclose(fp);
    printf("数据已保存到 %s\n", filename);
    return 1;
}
```

### 4.3 加载数据

```c
int load_data(ParkingSystem *ps, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        // 文件不存在不报错，使用初始状态
        return 0;
    }

    // 1. 读文件头
    FileHeader header;
    fread(&header, sizeof(FileHeader), 1, fp);

    // 校验魔数
    if (header.magic != FILE_MAGIC) {
        printf("文件格式错误！\n");
        fclose(fp);
        return 0;
    }

    // 2. 读车位状态
    fread(ps->spots, sizeof(ParkingSpot), FLOORS * ROWS * COLS, fp);

    // 3. 恢复空闲栈
    stack_init(&ps->free_spots, TOTAL_SPOTS);
    ps->free_spots.top = header.free_count - 1;
    fread(ps->free_spots.spots, sizeof(SpotLocation),
          header.free_count, fp);

    // 4. 恢复队列（逐个重建节点）
    queue_init(&ps->wait_queue);
    for (int i = 0; i < header.queue_count; i++) {
        QueueNode node;
        fread(&node, sizeof(QueueNode), 1, fp);
        queue_enqueue(&ps->wait_queue, node.plate, node.arrival_time);
    }

    // 5. 恢复历史记录
    ps->history_count = header.history_count;
    ps->history_capacity = header.history_count + 100;
    ps->history = (TransactionRecord *)malloc(
        sizeof(TransactionRecord) * ps->history_capacity);
    fread(ps->history, sizeof(TransactionRecord),
          header.history_count, fp);

    // 6. 恢复收入和计费规则
    ps->total_revenue = header.total_revenue;
    ps->fee_rule = header.fee_rule;

    fclose(fp);
    printf("数据已从 %s 加载\n", filename);
    return 1;
}
```

### 4.4 加载后重建哈希表

> 哈希表不持久化到文件，load 后需扫描 spots 重建。

```c
// 在 load_data 末尾或 ui_load 中重建哈希表
void rebuild_hash_from_spots(ParkingSystem *ps) {
    ht_clear(&ps->plate_index);
    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED) {
                    VehicleRecord *rec = malloc(sizeof(VehicleRecord));
                    strcpy(rec->plate, ps->spots[f][r][c].plate);
                    rec->loc = make_loc(f, r, c);
                    rec->entry_time = time(NULL); // 简化：以当前时间为入场时间
                    ht_insert(&ps->plate_index, rec);
                }
}
```

---

## 五、主程序入口 (main.c)

```c
#include "ds.h"
#include "parking.h"
#include "fee.h"
#include "stats.h"
#include "ui.h"
#include "file_io.h"

int main() {
    ParkingSystem ps;

    // 1. 初始化系统
    printf("正在初始化停车场系统...\n");
    ds_init(&ps);

    // 2. 尝试加载历史数据
    if (load_data(&ps, "parking_data.dat")) {
        // 重建哈希表（因为哈希表不是持久化的）
        for (int f = 0; f < FLOORS; f++)
            for (int r = 0; r < ROWS; r++)
                for (int c = 0; c < COLS; c++)
                    if (ps.spots[f][r][c].status == SPOT_OCCUPIED) {
                        VehicleRecord *rec = malloc(sizeof(VehicleRecord));
                        strcpy(rec->plate, ps.spots[f][r][c].plate);
                        rec->loc = make_loc(f, r, c);
                        rec->entry_time = time(NULL);
                        ht_insert(&ps.plate_index, rec);
                    }
    }

    printf("停车场初始化完成！共 %d 个车位\n", TOTAL_SPOTS);

    // 3. 进入主菜单
    ui_main_menu(&ps);

    // 4. 退出前保存
    printf("正在保存数据...\n");
    save_data(&ps, "parking_data.dat");

    // 5. 释放所有内存
    ds_destroy(&ps);
    printf("系统已安全退出\n");

    return 0;
}
```

---

## 六、依赖接口

### 6.1 调用 pzx 的接口

| 函数 | 功能 |
|------|------|
| `ds_init(ps)` | 初始化系统 |
| `ds_destroy(ps)` | 销毁系统，释放内存 |
| `stack_is_empty(s)` | 检查是否有空位 |
| `stack_size(s)` | 获取空闲车位数 |
| `queue_count(q)` | 获取排队人数 |
| `ht_find(ht, plate)` | 哈希表查找 |
| `make_loc(f, r, c)` | 创建坐标 |

### 6.2 调用 yjk 的接口

| 函数 | 功能 |
|------|------|
| `vehicle_entry(ps, plate)` | 入场登记 |
| `vehicle_exit(ps, plate, &fee)` | 出场结算 |
| `find_vehicle(ps, plate)` | 查找在场车辆 |
| `find_in_queue(ps, plate)` | 查找排队位置 |

---

## 七、开发顺序

| 阶段 | 任务 |
|------|------|
| **第一轮** | 完成 `ui.h` / `ui.c` 的菜单框架 + `file_io.h` / `file_io.c` 框架 |
| **第二轮** | 完成 `stats.c` + `file_io.c` 全部实现 + `main.c` |
| **第三轮** | 三人联调：编译所有文件，修复接口不匹配，测试验证 |

---

## 八、验收清单（zxc 负责项）

| # | 功能 | 说明 |
|---|------|------|
| 10 | 车位状态图 | ASCII 图显示占用/空闲 |
| 11 | 收入统计 | 总收入 / 今日 / 本月 |
| 12 | 交易明细 | 历史记录分页显示 |
| 13 | 楼层热度 | 各层周转率排名 |
| 14 | 峰值时段 | 各小时入场统计 |
| 15 | 数据持久化 | 二进制文件保存/加载 |
| 16 | 菜单界面 | 循环菜单，各功能入口 |
| 17 | 内存管理 | 与 pzx 共同确保无内存泄漏 |

---

## 九、代码规范（三人统一）

### 命名规范

| 类别 | 规范 | 示例 |
|------|------|------|
| 结构体 | 大驼峰 | `VehicleRecord`, `ParkingSpot` |
| 枚举 | 大写下划线 | `SPOT_FREE`, `ENTRY_SUCCESS` |
| 函数 | 小写下划线 | `stats_occupancy`, `ui_main_menu` |
| 变量 | 小写下划线 | `free_count`, `entry_time` |
| 宏 | 大写下划线 | `TOTAL_SPOTS`, `HT_SIZE` |
| 类型 | `typedef` + 大驼峰 | `ParkingSystem`, `HashMap` |

### 错误处理

1. `malloc` / `realloc` 后检查 `== NULL`
2. `fopen` 后检查 `== NULL`
3. 函数参数做基础校验（非 NULL）
4. 哈希表查找不到返回 NULL，不崩溃
5. 队列为空时 `dequeue` 返回 0，不崩溃

### 编译

```bash
gcc -o parking.exe ds.c parking.c fee.c stats.c ui.c file_io.c main.c -lm
```

- `-lm` 链接数学库（`ceil` 函数需要）
- 注意文件编码统一为 UTF-8 或 GBK（输出中文）
- Windows 下可能需添加 `#define _CRT_SECURE_NO_WARNINGS` 避免 scanf 警告
