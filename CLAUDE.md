# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

立体多层停车场管理系统 - 数据结构与算法课程综合性实验。C 语言实现，3 人协作。

停车场规模：3层 × 4行 × 5列 = 60 个车位。

## 编译与运行

源码位于 `最终提交材料/` 下的三个目录，项目根目录编译：

```bash
gcc -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=GBK \
  -I 最终提交材料/pzx -I 最终提交材料/yjk -I 最终提交材料/zxc \
  最终提交材料/pzx/ds.c 最终提交材料/yjk/parking.c 最终提交材料/yjk/fee.c \
  最终提交材料/zxc/stats.c 最终提交材料/zxc/ui.c 最终提交材料/zxc/file_io.c \
  最终提交材料/zxc/main.c -o parking.exe
```

Windows 控制台编码为 GBK（936），`-fexec-charset=GBK` 解决中文乱码。菜单边框用纯 ASCII（`+` `-` `|`）避免 Unicode 线条字符宽度不对齐。

## 架构：三层模块

依赖方向：`main → zxc(UI/stats/IO) → yjk(business) → pzx(data structures)`

| 层 | 目录 | 文件 | 职责 |
|---|---|---|---|
| 数据结构层 | `pzx/` | `ds.h`, `ds.c` | 核心结构体定义 + 栈/队列/哈希表实现 |
| 业务逻辑层 | `yjk/` | `parking.h/c`, `fee.h/c` | 车辆入场/出场/查找 + 费用计算 |
| 应用层 | `zxc/` | `main.c`, `ui.h/c`, `stats.h/c`, `file_io.h/c` | 主入口、CLI 菜单、统计、二进制文件持久化 |

## 核心数据结构（pzx/ds.h）

`ParkingSystem` 是全局上下文，聚合所有子结构，所有模块函数通过 `ParkingSystem *ps` 访问状态：

- `spots[3][4][5]` — 车位三维数组
- `free_spots` — 顺序栈，存空闲车位坐标，O(1) 分配/释放
- `wait_queue` — 链式 FIFO 队列，满位排队。节点是 `QueueNode`（车牌+到达时间+next指针）
- `plate_index` — 哈希表（拉链法，10007 桶），键=车牌，值=`VehicleRecord*`，O(1) 平均查询
- `history` — `TransactionRecord*` 动态数组，存历史交易
- `fee_rule` — 默认：免费 15 分钟、起步价 5 元、4 元/小时、日封顶 40 元
- `total_revenue` — 累计收入

## 关键业务流程

**入场** (`vehicle_entry`)：验证车牌 → 哈希表查重 → 有空位则 `stack_pop` 分配 + `ht_insert` 索引；满位则 `queue_enqueue` 排队。返回 `ENTRY_SUCCESS` / `PARKING_FULL` / `PLATE_EXISTS`。

**出场** (`vehicle_exit`)：`ht_find` 查找 → 计算时长 + `calculate_fee` 计费 → 生成 `TransactionRecord` → 从哈希表删除 → 车位释放：若队列非空则 `queue_dequeue` 自动调度队首车辆入场，否则 `stack_push` 回车位栈。

**计费** (`calculate_fee`)：时长 ≤ 15min 免费；否则 `起步价 + ceil(超出分钟/60) * 小时费率`，上限日封顶。

## 数据持久化（file_io.c）

二进制格式 `parking_data.dat`：文件头（魔数"PARK"+版本+时间戳+计数）→ 车位状态区 → 在场记录区 → 等待队列区 → 历史交易区 → 统计数据区。加载时需调用 `rebuild_hash_from_spots` 重建哈希索引。

## 菜单（zxc/ui.c）

13 个选项的 ASCII 文本菜单（`+--+` 边框），数字选择模式，每项对应一个 `ui_*` 函数。分页显示（历史交易每页 10 条，Q退出/N下一页/P上一页）。每项操作完成后等待按键返回主菜单。
