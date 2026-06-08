# yjk 负责部分和 yjk 部分代码逻辑详解

## 一、yjk 在三人分工中的定位

在《停车场管理系统_三人分工方案.md》中，系统整体被划分为三层：

| 成员 | 负责层次 | 主要文件 | 主要职责 |
|---|---|---|---|
| pzx | 数据结构基础层 | `ds.h`、`ds.c` | 定义系统核心结构体，实现栈、队列、哈希表、系统初始化和销毁 |
| yjk | 业务逻辑层 | `parking.h`、`parking.c`、`fee.h`、`fee.c` | 实现车辆入场、出场、查找、排队调度和停车计费 |
| zxc | 统计、界面和持久化层 | `stats.h/c`、`ui.h/c`、`file_io.h/c`、`main.c` | 实现菜单交互、统计分析、数据保存加载和程序入口 |

yjk 的部分处于系统中间层，向下依赖 pzx 提供的数据结构，向上为 zxc 的界面和主程序提供业务接口。

可以理解为：

```text
zxc：菜单、显示、统计、保存
        ↓ 调用
yjk：停车业务规则、入场出场、计费、排队调度
        ↓ 调用
pzx：栈、队列、哈希表、系统数据结构
```

因此，yjk 的代码不能只写简单函数，还必须承担“把底层数据结构组织成完整停车业务流程”的作用。

## 二、yjk 负责文件

当前 yjk 文件夹中包含以下文件：

```text
ljh/yjk
├── parking.h
├── parking.c
├── fee.h
├── fee.c
└── yjk负责部分和yjk部分代码逻辑.md
```

各文件作用如下：

| 文件 | 作用 |
|---|---|
| `parking.h` | 对外声明停车业务接口，包括车辆入场、车辆出场、车辆查找、排队查找 |
| `parking.c` | 实现停车业务流程，包括车位分配、出场结算、历史交易记录、排队自动调度 |
| `fee.h` | 对外声明计费接口 |
| `fee.c` | 实现停车费用计算和计费规则设置 |
| `yjk负责部分和yjk部分代码逻辑.md` | 说明 yjk 负责范围、业务逻辑、接口关系和测试方法 |

## 三、yjk 模块需要解决的业务问题

### 1. 车辆如何入场？

车辆到达入口后，系统需要完成以下判断：

1. 车牌号是否为空或过长。
2. 该车牌是否已经停在停车场内。
3. 该车牌是否已经在等待队列中。
4. 当前是否还有空闲车位。
5. 如果有空位，分配车位并记录入场信息。
6. 如果无空位，加入等待队列。

该功能由 `vehicle_entry` 实现。

### 2. 车辆如何出场？

车辆离开停车场时，系统需要完成以下处理：

1. 根据车牌号查找车辆是否在场。
2. 如果不在场，判断是否在等待队列中。
3. 如果车辆在场，计算停车时长。
4. 根据计费规则计算停车费用。
5. 生成历史交易记录。
6. 更新停车场总收入。
7. 释放车位。
8. 删除车牌索引。
9. 释放车辆记录内存。
10. 如果等待队列不为空，则队首车辆自动进入刚释放的车位。
11. 如果等待队列为空，则该车位重新进入空闲车位栈。

该功能由 `vehicle_exit` 实现。

### 3. 如何快速查找车辆？

对于在场车辆，系统通过 pzx 提供的哈希表快速查找。  
对于等待车辆，系统遍历等待队列并返回排队序号。

相关函数：

```c
VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate);
int find_in_queue(ParkingSystem *ps, const char *plate);
```

### 4. 如何计算停车费用？

系统根据停车时长和计费规则计算费用。默认规则来自 `ds_init`：

| 计费项 | 默认值 |
|---|---|
| 免费时长 | 15 分钟 |
| 起步价 | 5.0 元 |
| 每小时收费 | 4.0 元 |
| 每日封顶 | 40.0 元 |

计费函数：

```c
double calculate_fee(const FeeRule *rule, int duration_minutes);
```

## 四、yjk 对外接口设计

### 1. `parking.h`

```c
#ifndef PARKING_H
#define PARKING_H

#include "../pzx/ds.h"

EntryResult vehicle_entry(ParkingSystem *ps, const char *plate);
ExitResult vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee);
VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate);
int find_in_queue(ParkingSystem *ps, const char *plate);

#endif
```

#### 接口说明

| 函数 | 作用 | 主要调用者 |
|---|---|---|
| `vehicle_entry` | 车辆入场登记 | zxc 的 UI 菜单、主流程 |
| `vehicle_exit` | 车辆出场结算 | zxc 的 UI 菜单、主流程 |
| `find_vehicle` | 查询在场车辆 | zxc 的查询界面、统计辅助 |
| `find_in_queue` | 查询车辆排队位置 | zxc 的查询界面 |

### 2. `fee.h`

```c
#ifndef FEE_H
#define FEE_H

#include "../pzx/ds.h"

double calculate_fee(const FeeRule *rule, int duration_minutes);
void set_fee_rule(ParkingSystem *ps, FeeRule rule);

#endif
```

#### 接口说明

| 函数 | 作用 | 主要调用者 |
|---|---|---|
| `calculate_fee` | 根据停车时长计算费用 | `vehicle_exit`、统计或测试模块 |
| `set_fee_rule` | 修改系统计费规则 | 管理员设置功能、UI 模块 |

## 五、核心业务流程详解

## 5.1 车辆入场流程

函数原型：

```c
EntryResult vehicle_entry(ParkingSystem *ps, const char *plate);
```

### 5.1.1 参数说明

| 参数 | 类型 | 含义 |
|---|---|---|
| `ps` | `ParkingSystem *` | 系统全局状态指针 |
| `plate` | `const char *` | 车辆车牌号 |

### 5.1.2 返回值说明

| 返回值 | 含义 |
|---|---|
| `ENTRY_SUCCESS` | 入场成功，车辆获得车位 |
| `PARKING_FULL` | 停车场已满，车辆进入等待队列 |
| `PLATE_EXISTS` | 车牌非法、已在场或已在队列中 |

### 5.1.3 详细处理步骤

车辆入场时，系统按以下步骤执行：

1. 判断 `ps` 是否为空。
2. 判断 `plate` 是否为空。
3. 判断车牌长度是否在 `1 ~ 15` 范围内。
4. 调用 `ht_find(&ps->plate_index, plate)` 检查车辆是否已经在场。
5. 调用 `find_in_queue(ps, plate)` 检查车辆是否已经在等待队列中。
6. 如果车牌重复，则返回 `PLATE_EXISTS`。
7. 调用 `stack_is_empty(&ps->free_spots)` 判断是否有空闲车位。
8. 如果没有空闲车位：
   - 调用 `queue_enqueue(&ps->wait_queue, plate, time(NULL))`。
   - 将车辆加入等待队列。
   - 返回 `PARKING_FULL`。
9. 如果有空闲车位：
   - 调用 `stack_pop(&ps->free_spots)` 取出一个空闲车位。
   - 修改对应 `ParkingSpot` 状态为 `SPOT_OCCUPIED`。
   - 将车牌写入该车位的 `plate` 字段。
   - 创建 `VehicleRecord`。
   - 记录车牌、车位位置和入场时间。
   - 调用 `ht_insert(&ps->plate_index, rec)` 将车辆加入车牌索引。
   - 返回 `ENTRY_SUCCESS`。

### 5.1.4 入场流程图

```text
输入车牌
   ↓
校验车牌是否合法
   ↓
检查是否已经在场或排队
   ↓
是否有空闲车位？
   ├── 是：弹出空闲车位 → 创建在场记录 → 插入哈希表 → 入场成功
   └── 否：加入等待队列 → 返回停车场已满
```

### 5.1.5 设计理由

车辆入场最核心的要求是“快速分配车位”。  
pzx 使用空闲车位栈保存所有空位，因此 yjk 只需要调用 `stack_pop` 即可 O(1) 获得一个车位。

同时，车辆重复入场会导致数据混乱，所以入场前必须检查：

- 是否已经在哈希表中。
- 是否已经在等待队列中。

这样可以保证同一辆车在同一时刻只存在于一个业务状态中。

## 5.2 车辆出场流程

函数原型：

```c
ExitResult vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee);
```

### 5.2.1 参数说明

| 参数 | 类型 | 含义 |
|---|---|---|
| `ps` | `ParkingSystem *` | 系统全局状态指针 |
| `plate` | `const char *` | 出场车辆车牌号 |
| `out_fee` | `double *` | 输出停车费用 |

### 5.2.2 返回值说明

| 返回值 | 含义 |
|---|---|
| `PARK_EXIT_SUCCESS` | 出场成功 |
| `NOT_FOUND` | 未找到该车辆 |
| `NOT_PARKED` | 车辆在等待队列中，但尚未真正停车 |

### 5.2.3 详细处理步骤

车辆出场时，系统按以下步骤执行：

1. 如果 `out_fee` 不为空，先将 `*out_fee` 置为 `0.0`。
2. 校验 `ps` 和 `plate`。
3. 调用 `ht_find(&ps->plate_index, plate)` 查找车辆在场记录。
4. 如果没有找到：
   - 调用 `find_in_queue(ps, plate)` 判断车辆是否在等待队列中。
   - 如果在等待队列中，返回 `NOT_PARKED`。
   - 如果不在等待队列中，返回 `NOT_FOUND`。
5. 如果找到车辆记录：
   - 保存车辆所在车位 `released_loc`。
   - 获取当前时间 `now = time(NULL)`。
   - 使用 `difftime(now, rec->entry_time)` 计算停车时长。
   - 将秒数换算为分钟。
   - 调用 `calculate_fee(&ps->fee_rule, duration_minutes)` 计算费用。
   - 将费用写入 `out_fee`。
6. 调用内部逻辑追加历史交易记录：
   - 车牌号。
   - 停车位置。
   - 入场时间。
   - 出场时间。
   - 停车时长。
   - 收费金额。
7. 更新总收入：
   - `ps->total_revenue += fee`。
8. 释放车位：
   - 将车位状态改为 `SPOT_FREE`。
   - 清空车位中的车牌号。
9. 删除哈希表索引：
   - 调用 `ht_remove(&ps->plate_index, plate)`。
10. 释放车辆记录内存：
   - `free(rec)`。
11. 判断等待队列是否为空：
   - 如果不为空：
     - 调用 `queue_dequeue` 取出队首车辆。
     - 将队首车辆直接安排到刚释放的车位。
     - 创建新的 `VehicleRecord`。
     - 插入哈希表。
   - 如果为空：
     - 调用 `stack_push(&ps->free_spots, released_loc)`。
     - 将车位重新放回空闲车位栈。
12. 返回 `PARK_EXIT_SUCCESS`。

### 5.2.4 出场流程图

```text
输入车牌
   ↓
查找哈希表中的在场记录
   ↓
是否在场？
   ├── 否：检查是否在等待队列
   │        ├── 在队列：返回 NOT_PARKED
   │        └── 不存在：返回 NOT_FOUND
   ↓
计算停车时长
   ↓
计算停车费用
   ↓
生成历史交易记录
   ↓
更新总收入
   ↓
释放车位并删除车辆记录
   ↓
等待队列是否为空？
   ├── 不为空：队首车辆自动入场，占用刚释放的车位
   └── 为空：释放车位压回空闲车位栈
   ↓
出场成功
```

### 5.2.5 设计理由

出场逻辑比入场更复杂，因为它不仅要处理当前车辆，还要处理等待车辆调度。

设计重点有三个：

1. **费用必须在释放记录前计算**  
   因为停车时长依赖 `rec->entry_time`，历史交易也依赖 `rec` 中的车牌和位置。

2. **历史交易必须在释放车辆记录前生成**  
   如果先释放 `rec`，交易记录会丢失必要信息。

3. **出场后优先处理等待队列**  
   如果停车场满位时已有车辆等待，那么释放出的车位不应该先进入空闲栈，而应该直接分配给队首车辆，实现“出一个进一个”。

## 六、计费模块详解

## 6.1 费用计算函数

函数原型：

```c
double calculate_fee(const FeeRule *rule, int duration_minutes);
```

### 6.1.1 参数说明

| 参数 | 类型 | 含义 |
|---|---|---|
| `rule` | `const FeeRule *` | 计费规则 |
| `duration_minutes` | `int` | 停车时长，单位为分钟 |

### 6.1.2 计费规则

当前实现的计费逻辑如下：

```text
如果停车时长 <= 免费时长：
    费用 = 0
否则：
    超出分钟数 = 停车时长 - 免费时长
    超出小时数 = ceil(超出分钟数 / 60.0)
    费用 = 起步价 + 超出小时数 × 每小时收费
    如果费用 > 每日封顶：
        费用 = 每日封顶
```

### 6.1.3 示例

假设计费规则为：

- 免费 15 分钟。
- 起步价 5 元。
- 每小时 4 元。
- 每日封顶 40 元。

则：

| 停车时长 | 计算过程 | 费用 |
|---|---|---|
| 10 分钟 | 小于免费时长 | 0 元 |
| 15 分钟 | 等于免费时长 | 0 元 |
| 16 分钟 | 起步价 5 + 1 小时 4 | 9 元 |
| 75 分钟 | 超出 60 分钟，按 1 小时 | 9 元 |
| 76 分钟 | 超出 61 分钟，按 2 小时 | 13 元 |
| 很长时间 | 超过封顶价 | 40 元 |

### 6.1.4 为什么使用 `ceil`

停车计费通常按照“不足一小时按一小时计算”的方式处理。  
因此只要超出免费时间，即使只多 1 分钟，也按照 1 小时计算。

例如：

```text
停车 16 分钟：
免费 15 分钟
超出 1 分钟
ceil(1 / 60.0) = 1 小时
```

## 6.2 修改计费规则函数

函数原型：

```c
void set_fee_rule(ParkingSystem *ps, FeeRule rule);
```

该函数用于系统管理员修改停车场收费规则。  
例如节假日或不同停车场可以调整起步价、免费时间、小时费率和封顶金额。

当前实现中只进行简单赋值：

```c
ps->fee_rule = rule;
```

如果后续扩展，可以增加规则合法性检查，例如：

- 免费时长不能为负数。
- 起步价不能为负数。
- 每小时收费不能为负数。
- 封顶金额不能小于起步价。

## 七、内部辅助函数说明

yjk 的 `parking.c` 中除了对外接口，还设计了几个内部辅助函数。这些函数使用 `static` 修饰，只在当前源文件中可见，避免污染其他模块。

### 1. `is_valid_plate`

作用：

```c
static int is_valid_plate(const char *plate);
```

用于检查车牌是否合法。

当前判断条件：

- `plate != NULL`
- 长度大于 0
- 长度小于 16

设计意义：

- 避免空车牌进入系统。
- 避免写入 `char plate[16]` 时越界。

### 2. `copy_plate`

作用：

```c
static void copy_plate(char dest[16], const char *src);
```

用于安全复制车牌号。

当前使用：

```c
strncpy(dest, src, 15);
dest[15] = '\0';
```

设计意义：

- 防止 `strcpy` 因输入过长造成数组越界。
- 保证目标字符串一定以 `'\0'` 结尾。

### 3. `create_vehicle_record`

作用：

```c
static VehicleRecord *create_vehicle_record(const char *plate,
                                            SpotLocation loc,
                                            time_t entry_time);
```

用于创建一条在场车辆记录。

主要步骤：

1. 动态申请 `VehicleRecord` 内存。
2. 写入车牌。
3. 写入车位位置。
4. 写入入场时间。
5. 返回指针。

设计意义：

- 避免在多个位置重复写创建记录的代码。
- 保证入场车辆和排队车辆自动入场时使用同一套创建逻辑。

### 4. `occupy_spot`

作用：

```c
static void occupy_spot(ParkingSystem *ps,
                        SpotLocation loc,
                        const char *plate,
                        time_t entry_time);
```

用于把某个车位设置为占用状态，并为车辆创建在场记录。

主要步骤：

1. 创建 `VehicleRecord`。
2. 修改对应车位状态为 `SPOT_OCCUPIED`。
3. 写入车牌号。
4. 将车辆记录插入哈希表。

设计意义：

- 入场成功和排队车辆自动入场都会占用车位。
- 使用统一函数可减少重复代码。

### 5. `ensure_history_capacity`

作用：

```c
static void ensure_history_capacity(ParkingSystem *ps);
```

用于保证历史交易数组有足够容量。

当 `history_count >= history_capacity` 时，系统将历史数组扩容为原来的 2 倍。

设计意义：

- 历史交易数量不可预知。
- 动态扩容能保证系统持续记录交易。

### 6. `append_transaction`

作用：

```c
static void append_transaction(ParkingSystem *ps,
                               const VehicleRecord *rec,
                               time_t exit_time,
                               int duration_minutes,
                               double fee);
```

用于追加一条历史交易记录。

记录内容包括：

- 车牌号。
- 停车位置。
- 入场时间。
- 出场时间。
- 停车时长。
- 停车费用。

设计意义：

- 出场结算后必须保存交易信息。
- 统计收入、历史查询、高峰分析都依赖该数据。

## 八、与 pzx 模块的关系

yjk 模块直接依赖 pzx 的 `ds.h`。

### 8.1 yjk 调用 pzx 的数据结构

| pzx 数据结构 | yjk 使用方式 |
|---|---|
| `ParkingSystem` | 所有业务函数都围绕该结构体操作 |
| `ParkingSpot` | 入场和出场时修改车位状态 |
| `SpotLocation` | 表示分配或释放的车位 |
| `VehicleRecord` | 保存车辆在场记录 |
| `FreeSpotStack` | 入场分配空位，出场释放空位 |
| `WaitingQueue` | 满位时排队，释放车位后自动调度 |
| `HashMap` | 根据车牌快速查找车辆 |
| `TransactionRecord` | 出场后追加历史记录 |
| `FeeRule` | 出场时计算停车费用 |

### 8.2 yjk 调用 pzx 的函数

| pzx 函数 | yjk 调用场景 |
|---|---|
| `stack_is_empty` | 判断是否有空闲车位 |
| `stack_pop` | 车辆入场时取出一个空闲车位 |
| `stack_push` | 出场后且无等待车辆时释放车位 |
| `queue_enqueue` | 停车场满位时车辆加入等待队列 |
| `queue_dequeue` | 有车位释放后取出队首车辆 |
| `queue_is_empty` | 判断是否存在等待车辆 |
| `ht_find` | 查询车辆是否在场 |
| `ht_insert` | 入场成功后建立车牌索引 |
| `ht_remove` | 出场后删除车牌索引 |

## 九、与 zxc 模块的关系

zxc 负责 UI、统计、文件保存和主函数。  
yjk 为 zxc 提供可直接调用的业务接口。

### 9.1 zxc 调用 yjk 的典型方式

#### 车辆入场菜单

```c
char plate[16];
scanf("%15s", plate);
EntryResult result = vehicle_entry(&ps, plate);
```

根据返回值显示：

| 返回值 | UI 显示内容 |
|---|---|
| `ENTRY_SUCCESS` | 入场成功，显示分配车位 |
| `PARKING_FULL` | 停车场已满，车辆已加入等待队列 |
| `PLATE_EXISTS` | 车牌重复或非法 |

#### 车辆出场菜单

```c
char plate[16];
double fee;
ExitResult result = vehicle_exit(&ps, plate, &fee);
```

根据返回值显示：

| 返回值 | UI 显示内容 |
|---|---|
| `PARK_EXIT_SUCCESS` | 出场成功，显示收费金额 |
| `NOT_FOUND` | 未找到该车辆 |
| `NOT_PARKED` | 车辆正在排队，尚未停车 |

#### 查询菜单

```c
VehicleRecord *rec = find_vehicle(&ps, plate);
int pos = find_in_queue(&ps, plate);
```

如果 `rec != NULL`，说明车辆在场。  
如果 `pos != -1`，说明车辆在等待队列中。

## 十、关键数据变化过程

## 10.1 正常入场时的数据变化

假设车辆 `A12345` 入场且有空位。

变化过程：

```text
free_spots 栈数量 -1
spots[f][r][c].status = SPOT_OCCUPIED
spots[f][r][c].plate = "A12345"
创建 VehicleRecord
plate_index 插入 "A12345" -> VehicleRecord*
wait_queue 不变
history 不变
total_revenue 不变
```

## 10.2 满位入场时的数据变化

假设车辆 `B12345` 入场时停车场已满。

变化过程：

```text
free_spots 栈数量 = 0
spots 不变
plate_index 不变
wait_queue 增加一个 QueueNode
history 不变
total_revenue 不变
```

## 10.3 出场且无人等待时的数据变化

假设车辆 `A12345` 出场，等待队列为空。

变化过程：

```text
计算停车费用
history 增加一条 TransactionRecord
total_revenue 增加 fee
spots[f][r][c].status = SPOT_FREE
spots[f][r][c].plate = ""
plate_index 删除 "A12345"
释放 VehicleRecord
free_spots 栈数量 +1
wait_queue 不变
```

## 10.4 出场且有人等待时的数据变化

假设车辆 `A12345` 出场，等待队列中有 `B12345`。

变化过程：

```text
A12345 计算费用并生成历史交易
A12345 从 plate_index 删除
A12345 的 VehicleRecord 被释放
等待队列队首 B12345 出队
B12345 直接占用 A12345 刚释放的车位
为 B12345 创建 VehicleRecord
plate_index 插入 "B12345"
free_spots 栈数量不变
```

注意：

如果等待队列不为空，释放的车位不会先压回 `free_spots`。  
这是为了保证排队车辆优先获得车位。

## 十一、边界情况处理

### 1. 空指针

如果 `ParkingSystem *ps` 为空，业务函数不会继续操作。

处理方式：

- `vehicle_entry` 返回 `PLATE_EXISTS`。
- `vehicle_exit` 返回 `NOT_FOUND`。
- `find_vehicle` 返回 `NULL`。
- `find_in_queue` 返回 `-1`。

### 2. 非法车牌

非法情况包括：

- `plate == NULL`
- 车牌长度为 0
- 车牌长度大于等于 16

处理方式：

- 入场视为非法，返回 `PLATE_EXISTS`。
- 出场视为找不到，返回 `NOT_FOUND`。

### 3. 重复入场

如果车牌已经在哈希表中，说明车辆已经在场。  
如果车牌已经在等待队列中，说明车辆已经排队。  
这两种情况都返回 `PLATE_EXISTS`。

### 4. 出场车辆不存在

如果车辆既不在哈希表，也不在等待队列中，返回 `NOT_FOUND`。

### 5. 排队车辆尝试出场

如果车辆在等待队列中，但没有真正进入停车位，则返回 `NOT_PARKED`。

### 6. 停车时长为负

理论上系统时间可能被用户修改，导致当前时间早于入场时间。  
当前实现中如果时长小于 0，则将停车时长修正为 0。

### 7. 历史交易容量不足

当历史交易数量达到容量上限时，自动扩容。

```text
new_capacity = old_capacity * 2
```

如果原容量为 0，则扩容到 100。

## 十二、算法复杂度分析

| 操作 | 使用的数据结构 | 时间复杂度 | 说明 |
|---|---|---|---|
| 判断是否有空位 | 空闲车位栈 | O(1) | 判断 `top` |
| 分配车位 | 空闲车位栈 | O(1) | `stack_pop` |
| 释放车位 | 空闲车位栈 | O(1) | `stack_push` |
| 等待车辆入队 | 链式队列 | O(1) | 尾插法 |
| 等待车辆出队 | 链式队列 | O(1) | 删除队头 |
| 查找在场车辆 | 哈希表 | 平均 O(1) | 根据车牌查找 |
| 删除在场车辆 | 哈希表 | 平均 O(1) | 根据车牌删除 |
| 查找排队车辆 | 链式队列 | O(n) | 需要遍历队列 |
| 追加历史交易 | 动态数组 | 均摊 O(1) | 容量不足时扩容 |
| 计算费用 | 普通计算 | O(1) | 常数次运算 |

整体来看，系统最常用的入场、出场和查找在场车辆操作都能保持较高效率。

## 十三、内存管理说明

yjk 模块中涉及动态内存的主要对象是 `VehicleRecord` 和历史交易数组。

### 1. `VehicleRecord` 的生命周期

创建位置：

```c
create_vehicle_record(...)
```

创建时机：

- 车辆成功入场。
- 等待队列车辆自动获得车位。

插入位置：

```c
ht_insert(&ps->plate_index, rec);
```

释放时机：

```c
ht_remove(&ps->plate_index, plate);
free(rec);
```

也就是说：

```text
VehicleRecord 创建于入场，销毁于出场。
```

### 2. 历史交易数组的扩容

历史交易数组由 pzx 的 `ds_init` 初始化容量。  
yjk 在出场时追加历史记录，如果容量不足，则调用 `realloc` 扩容。

### 3. 为什么 `ht_remove` 不释放 `VehicleRecord`

pzx 的哈希表删除函数只释放 `HTNode`，不释放 `VehicleRecord`。  
这是合理的，因为 `VehicleRecord` 的生命周期由业务层 yjk 管理。

所以出场时必须执行：

```c
ht_remove(&ps->plate_index, plate);
free(rec);
```

如果只调用 `ht_remove` 而不 `free(rec)`，会造成内存泄漏。

## 十四、当前 yjk 代码的优点

1. **业务职责清晰**  
   yjk 只负责停车业务和计费，不实现 UI，不实现底层数据结构。

2. **接口符合三人分工**  
   对外提供 `vehicle_entry`、`vehicle_exit`、`find_vehicle`、`find_in_queue`、`calculate_fee`、`set_fee_rule`。

3. **入场和出场流程完整**  
   覆盖有空位、无空位、重复车牌、出场结算、排队调度等场景。

4. **充分利用 pzx 数据结构**  
   通过栈实现 O(1) 车位分配，通过哈希表实现 O(1) 平均查找，通过队列实现 FIFO 排队。

5. **有历史交易记录**  
   出场时生成 `TransactionRecord`，便于 zxc 后续统计和显示。

6. **有基本边界检查**  
   对空指针、非法车牌、重复车牌等情况进行了处理。

## 十五、当前 yjk 代码可以继续完善的地方

### 1. 入场非法车牌返回值可更精确

当前 `EntryResult` 中没有专门表示非法车牌的枚举值，所以非法车牌暂时返回 `PLATE_EXISTS`。

如果后续扩展，可以增加：

```c
INVALID_PLATE
```

### 2. 出场时排队车辆自动入场的时间选择

当前排队车辆获得车位时，入场时间设置为当前出场时间 `now`。

这样表示：

```text
车辆真正获得车位的时间才算停车开始时间。
```

这是比较合理的。  
如果想把等待时间也计入停车时间，则可以改为使用 `queue_arrival_time`。

### 3. 计费规则合法性检查可以更严格

目前 `set_fee_rule` 只是简单赋值。  
后续可以检查：

- 起步价是否非负。
- 免费时长是否非负。
- 每小时费率是否非负。
- 封顶金额是否合理。

### 4. 查询排队车辆是 O(n)

等待队列查找需要遍历队列。  
如果排队车辆非常多，可以增加一个“排队车辆哈希索引”。  
但对于本课程实践系统，当前实现已经足够。

## 十六、建议测试场景

### 测试 1：普通入场

操作：

```text
车辆 A001 入场
```

预期：

```text
返回 ENTRY_SUCCESS
空闲车位数减少 1
哈希表中可以查到 A001
对应车位状态为 SPOT_OCCUPIED
```

### 测试 2：重复入场

操作：

```text
A001 已经在场
再次执行 A001 入场
```

预期：

```text
返回 PLATE_EXISTS
空闲车位数不变
等待队列不变
```

### 测试 3：停车场满位

操作：

```text
连续入场 60 辆车
第 61 辆车入场
```

预期：

```text
前 60 辆返回 ENTRY_SUCCESS
第 61 辆返回 PARKING_FULL
等待队列数量为 1
```

### 测试 4：出场无人等待

操作：

```text
A001 出场，等待队列为空
```

预期：

```text
返回 PARK_EXIT_SUCCESS
生成历史交易
总收入增加
空闲车位数增加 1
哈希表中查不到 A001
```

### 测试 5：出场后自动调度

操作：

```text
停车场已满
B001 在等待队列中
A001 出场
```

预期：

```text
A001 出场成功
B001 自动进入 A001 释放的车位
等待队列数量减少 1
空闲车位数仍为 0
哈希表中可以查到 B001
```

### 测试 6：查找车辆

操作：

```text
查询在场车辆 A001
查询排队车辆 B001
查询不存在车辆 C001
```

预期：

```text
A001 返回 VehicleRecord*
B001 返回排队序号
C001 返回 NULL 或 -1
```

### 测试 7：计费

操作：

```text
calculate_fee(rule, 15)
calculate_fee(rule, 16)
calculate_fee(rule, 76)
```

预期：

```text
15 分钟：0 元
16 分钟：9 元
76 分钟：13 元
```

## 十七、yjk 模块编译方式

因为 `fee.c` 中使用了 `ceil`，完整编译时需要链接数学库。

Linux / MinGW 下可使用：

```bash
gcc -Wall -Wextra -pedantic ../pzx/ds.c parking.c fee.c -lm -o test_yjk.exe
```

如果与 zxc 的主程序一起编译：

```bash
gcc -o parking.exe \
    ../pzx/ds.c \
    parking.c fee.c \
    ../zxc/stats.c ../zxc/ui.c ../zxc/file_io.c ../zxc/main.c \
    -lm
```

具体路径可根据最终目录调整。

## 十八、yjk 模块总结

yjk 模块是停车场管理系统的核心业务逻辑层。  
它不直接负责界面显示，也不直接负责底层数据结构实现，而是把 pzx 提供的栈、队列、哈希表和系统结构组织成完整的停车业务流程。

yjk 的核心贡献包括：

1. 实现车辆入场登记。
2. 实现空闲车位自动分配。
3. 实现停车场满位时排队。
4. 实现车辆出场结算。
5. 实现停车费用计算。
6. 实现出场后等待车辆自动调度。
7. 实现车辆实时查找。
8. 实现历史交易记录追加。
9. 实现收入累计更新。

从系统整体来看，pzx 提供“能存什么、怎么存”，yjk 负责“业务怎么流转”，zxc 负责“用户怎么操作和查看”。  
因此，yjk 模块是连接底层数据结构和用户操作界面的关键中间层。
