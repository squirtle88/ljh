# 立体多层停车场管理系统数据对象分析与 ADT 设计

## 一、分析目标

根据系统业务需求和三人分工方案，本系统需要完成车辆入场、车位分配、满位排队、车辆出场、费用结算、实时查找、收入统计和数据持久化等业务。因此，程序需要处理的数据对象不仅包括车辆和车位，还包括在场记录、等待队列、空闲车位栈、车牌索引、计费规则、历史交易记录和系统全局状态。

本系统采用 C 语言实现，数据结构设计主要由 pzx 负责，业务逻辑由 yjk 负责，统计、界面和文件持久化由 zxc 负责。

## 二、程序要处理的数据对象

系统需要处理的数据对象如下：

| 序号 | 数据对象 | 对应结构体 / 类型 | 主要作用 |
|---|---|---|---|
| 1 | 车位位置对象 | `SpotLocation` | 表示车位所在楼层、行、列 |
| 2 | 车位对象 | `ParkingSpot` | 表示单个车位的状态和占用车辆 |
| 3 | 在场车辆记录对象 | `VehicleRecord` | 记录已停车车辆的车牌、车位和入场时间 |
| 4 | 空闲车位栈对象 | `FreeSpotStack` | 保存所有空闲车位，实现 O(1) 车位分配和释放 |
| 5 | 等待队列节点对象 | `QueueNode` | 保存等待入场车辆的车牌和到达时间 |
| 6 | 等待队列对象 | `WaitingQueue` | 管理满位时等待车辆，采用 FIFO 逻辑 |
| 7 | 哈希表节点对象 | `HTNode` | 保存车牌到在场记录的映射节点 |
| 8 | 车牌索引对象 | `HashMap` | 根据车牌快速查找在场车辆记录 |
| 9 | 计费规则对象 | `FeeRule` | 保存停车收费规则 |
| 10 | 历史交易记录对象 | `TransactionRecord` | 记录车辆出场后的完整交易信息 |
| 11 | 系统全局状态对象 | `ParkingSystem` | 聚合系统运行所需的全部数据 |
| 12 | 统计数据对象 | 由 `history` 和 `spots` 派生 | 用于收入统计、楼层热度和高峰时段分析 |

## 三、各数据对象的数据项组成

### 1. 车位位置对象 `SpotLocation`

`SpotLocation` 用于唯一标识一个车位在立体停车场中的空间位置。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `floor` | 楼层编号 | `int` | `0 ~ FLOORS-1`，本系统为 `0 ~ 2` |
| `row` | 行编号 | `int` | `0 ~ ROWS-1`，本系统为 `0 ~ 3` |
| `col` | 列编号 | `int` | `0 ~ COLS-1`，本系统为 `0 ~ 4` |

结构特点：

- 三元组结构。
- 与 `ParkingSpot`、`VehicleRecord`、`TransactionRecord` 均有关联。
- 程序内部从 0 开始编号，显示给用户时可转换为 1 开始编号。

ADT 描述：

```c
ADT SpotLocation {
    数据对象：车位三维坐标集合。
    数据关系：floor、row、col 组成一个有序三元组。
    基本操作：
        SpotLocation make_loc(int f, int r, int c);
        int loc_equal(SpotLocation a, SpotLocation b);
}
```

### 2. 车位对象 `ParkingSpot`

`ParkingSpot` 表示停车场中的一个具体车位。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `loc` | 车位位置 | `SpotLocation` | 合法楼层、行、列组合 |
| `status` | 车位状态 | `SpotStatus` | `SPOT_FREE` 或 `SPOT_OCCUPIED` |
| `plate` | 占用车辆车牌 | `char[16]` | 空字符串或长度 1~15 的车牌号 |

结构特点：

- 每个车位有唯一位置。
- 空闲时 `plate` 为空字符串。
- 占用时 `plate` 保存车辆车牌。
- 所有车位组成三维数组 `spots[FLOORS][ROWS][COLS]`。

ADT 描述：

```c
ADT ParkingSpot {
    数据对象：停车场中的单个车位。
    数据关系：一个车位对应一个固定 SpotLocation；同一时刻最多被一辆车占用。
    基本操作：
        设置车位为空闲；
        设置车位为占用；
        查询车位状态；
}
```

### 3. 在场车辆记录对象 `VehicleRecord`

`VehicleRecord` 用于记录当前已停入车位的车辆。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `plate` | 车牌号 | `char[16]` | 非空，长度 1~15 |
| `loc` | 停放位置 | `SpotLocation` | 当前车辆占用的合法车位 |
| `entry_time` | 入场时间 | `time_t` | 合法系统时间 |

结构特点：

- 只保存当前在场车辆。
- 由哈希表 `HashMap` 根据车牌索引。
- 出场后从哈希表中删除，并转化为历史交易记录。

ADT 描述：

```c
ADT VehicleRecord {
    数据对象：当前在场车辆记录集合。
    数据关系：每个车牌至多对应一个 VehicleRecord。
    基本操作：
        创建在场车辆记录；
        根据车牌查询记录；
        删除在场车辆记录；
}
```

### 4. 空闲车位栈对象 `FreeSpotStack`

`FreeSpotStack` 保存当前所有空闲车位的位置。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `spots` | 存放空闲车位坐标的动态数组 | `SpotLocation *` | 指向动态分配空间 |
| `top` | 栈顶下标 | `int` | `-1 ~ capacity-1` |
| `capacity` | 栈容量 | `int` | 正整数，可扩容 |

结构特点：

- 顺序栈结构。
- 入栈和出栈均为 O(1)。
- 入场时 `pop` 一个空位，出场且无等待车辆时 `push` 回空位。

ADT 描述：

```c
ADT FreeSpotStack {
    数据对象：所有空闲车位坐标集合。
    数据关系：后进先出 LIFO。
    基本操作：
        void stack_init(FreeSpotStack *s, int capacity);
        void stack_destroy(FreeSpotStack *s);
        void stack_push(FreeSpotStack *s, SpotLocation loc);
        SpotLocation stack_pop(FreeSpotStack *s);
        int stack_is_empty(const FreeSpotStack *s);
        int stack_size(const FreeSpotStack *s);
}
```

### 5. 等待队列节点对象 `QueueNode`

`QueueNode` 表示一辆等待入场的车辆。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `plate` | 等待车辆车牌 | `char[16]` | 非空，长度 1~15 |
| `arrival_time` | 到达等待队列时间 | `time_t` | 合法系统时间 |
| `next` | 后继节点指针 | `struct QueueNode *` | `NULL` 或下一节点地址 |

结构特点：

- 链式队列节点。
- 不包含车位信息，因为等待车辆尚未分配车位。

### 6. 等待队列对象 `WaitingQueue`

`WaitingQueue` 管理停车场满位时的等待车辆。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `front` | 队头指针 | `QueueNode *` | `NULL` 或队头节点 |
| `rear` | 队尾指针 | `QueueNode *` | `NULL` 或队尾节点 |
| `count` | 队列长度 | `int` | `0` 或正整数 |

结构特点：

- 链式 FIFO 队列。
- 队头车辆优先获得释放车位。
- 入队和出队均为 O(1)。

ADT 描述：

```c
ADT WaitingQueue {
    数据对象：等待入场车辆集合。
    数据关系：先进先出 FIFO。
    基本操作：
        void queue_init(WaitingQueue *q);
        void queue_clear(WaitingQueue *q);
        void queue_enqueue(WaitingQueue *q, const char *plate, time_t t);
        int queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t);
        int queue_is_empty(const WaitingQueue *q);
        int queue_count(const WaitingQueue *q);
        int find_in_queue(ParkingSystem *ps, const char *plate);
}
```

### 7. 哈希表节点对象 `HTNode`

`HTNode` 用于哈希表拉链法处理冲突。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `plate` | 车牌号 | `char[16]` | 非空，长度 1~15 |
| `record` | 在场车辆记录指针 | `VehicleRecord *` | 指向对应在场记录 |
| `next` | 同桶下一个节点 | `struct HTNode *` | `NULL` 或下一节点地址 |

结构特点：

- 一个节点保存一个车牌索引项。
- `record` 指向 `VehicleRecord`，不直接保存完整车辆数据。

### 8. 车牌索引对象 `HashMap`

`HashMap` 用于根据车牌快速查找车辆。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `buckets` | 哈希桶数组 | `HTNode *[HT_SIZE]` | 每个桶为空或链表头 |
| `count` | 当前索引记录数 | `int` | `0 ~ TOTAL_SPOTS` |

结构特点：

- 使用拉链法哈希表。
- 哈希函数以车牌字符串为关键字。
- 平均查找、插入、删除复杂度为 O(1)。

ADT 描述：

```c
ADT HashMap {
    数据对象：车牌到 VehicleRecord 的映射集合。
    数据关系：同一车牌唯一映射到一个在场车辆记录。
    基本操作：
        unsigned int hash_str(const char *str);
        void ht_init(HashMap *ht);
        void ht_insert(HashMap *ht, VehicleRecord *rec);
        VehicleRecord *ht_find(HashMap *ht, const char *plate);
        void ht_remove(HashMap *ht, const char *plate);
        void ht_clear(HashMap *ht);
}
```

### 9. 计费规则对象 `FeeRule`

`FeeRule` 保存停车收费标准。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `base_fee` | 起步价 | `double` | `>= 0` |
| `free_minutes` | 免费时长 | `int` | `>= 0` |
| `rate_per_hour` | 每小时收费 | `double` | `>= 0` |
| `max_daily` | 每日封顶金额 | `double` | `>= 0`，0 可表示不封顶 |

结构特点：

- 由系统管理员维护。
- 出场结算时由计费模块读取。

ADT 描述：

```c
ADT FeeRule {
    数据对象：停车计费规则。
    数据关系：停车时长依据该规则映射为停车费用。
    基本操作：
        double calculate_fee(const FeeRule *rule, int duration_minutes);
        void set_fee_rule(ParkingSystem *ps, FeeRule rule);
}
```

### 10. 历史交易记录对象 `TransactionRecord`

`TransactionRecord` 保存车辆出场后的交易结果。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `plate` | 车牌号 | `char[16]` | 非空，长度 1~15 |
| `loc` | 停车位置 | `SpotLocation` | 曾经停放的合法车位 |
| `entry_time` | 入场时间 | `time_t` | 合法系统时间 |
| `exit_time` | 出场时间 | `time_t` | 不早于入场时间 |
| `duration_minutes` | 停车时长 | `int` | `>= 0` |
| `fee` | 停车费用 | `double` | `>= 0` |

结构特点：

- 历史记录采用动态数组保存。
- 车辆出场时产生一条交易记录。
- 统计模块基于历史交易记录计算收入和高峰数据。

ADT 描述：

```c
ADT TransactionHistory {
    数据对象：所有已完成停车交易记录集合。
    数据关系：线性表，按交易产生时间追加。
    基本操作：
        追加交易记录；
        查询历史交易；
        统计收入；
        分析高峰时段；
}
```

### 11. 系统全局状态对象 `ParkingSystem`

`ParkingSystem` 是系统运行时的总控数据对象。

| 数据项 | 含义 | 类型 | 值范围 |
|---|---|---|---|
| `spots` | 全部车位 | `ParkingSpot[FLOORS][ROWS][COLS]` | 固定 60 个车位 |
| `plate_index` | 车牌索引 | `HashMap` | 当前在场车辆索引 |
| `free_spots` | 空闲车位栈 | `FreeSpotStack` | 当前空闲车位 |
| `wait_queue` | 等待队列 | `WaitingQueue` | 当前排队车辆 |
| `history` | 历史交易数组 | `TransactionRecord *` | 动态数组 |
| `history_count` | 当前历史记录数 | `int` | `>= 0` |
| `history_capacity` | 历史数组容量 | `int` | `>= history_count` |
| `fee_rule` | 计费规则 | `FeeRule` | 有效收费规则 |
| `total_revenue` | 累计收入 | `double` | `>= 0` |

结构特点：

- 聚合所有核心数据对象。
- 其他模块函数通常以 `ParkingSystem *ps` 作为参数。
- 是系统初始化、业务处理、统计和持久化的核心上下文。

ADT 描述：

```c
ADT ParkingSystem {
    数据对象：停车场运行状态集合。
    数据关系：聚合车位、索引、队列、栈、计费规则和历史交易。
    基本操作：
        void ds_init(ParkingSystem *ps);
        void ds_destroy(ParkingSystem *ps);
        EntryResult vehicle_entry(ParkingSystem *ps, const char *plate);
        ExitResult vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee);
        VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate);
}
```

## 四、数据元素之间的逻辑结构

### 1. 车位集合的逻辑结构

车位集合采用三维数组：

```c
ParkingSpot spots[FLOORS][ROWS][COLS];
```

逻辑结构特点：

- 属于多维数组结构。
- 每个元素通过 `(floor, row, col)` 唯一定位。
- 支持按楼层、行、列遍历显示车位状态。

### 2. 空闲车位集合的逻辑结构

空闲车位采用顺序栈：

```c
FreeSpotStack free_spots;
```

逻辑结构特点：

- 栈结构，后进先出。
- 入场分配车位时执行 `stack_pop`。
- 出场释放车位且无等待车辆时执行 `stack_push`。
- 保证车位分配和释放时间复杂度为 O(1)。

### 3. 等待车辆集合的逻辑结构

等待车辆采用链式队列：

```c
WaitingQueue wait_queue;
```

逻辑结构特点：

- 队列结构，先进先出。
- 停车场满位时车辆入队。
- 释放车位后队头车辆出队并自动分配车位。
- 保证排队公平。

### 4. 在场车辆索引的逻辑结构

在场车辆索引采用哈希表：

```c
HashMap plate_index;
```

逻辑结构特点：

- 关键字为车牌号。
- 值为 `VehicleRecord *`。
- 使用拉链法解决哈希冲突。
- 平均查找复杂度为 O(1)。

### 5. 历史交易集合的逻辑结构

历史交易采用动态顺序表：

```c
TransactionRecord *history;
```

逻辑结构特点：

- 按交易产生顺序追加。
- 容量不足时扩容。
- 适合统计收入、查询历史和分析高峰时段。

## 五、数据对象之间的参照性和依赖关系

系统中各数据对象之间存在如下关系：

1. `ParkingSystem` 聚合全部核心数据对象。
2. `ParkingSpot` 通过 `loc` 引用 `SpotLocation`。
3. `VehicleRecord` 通过 `loc` 引用车辆当前停放的 `ParkingSpot`。
4. `HashMap` 通过 `HTNode.record` 指向 `VehicleRecord`，用于车牌快速查找。
5. `FreeSpotStack` 保存当前所有空闲车位的 `SpotLocation`。
6. `WaitingQueue` 保存未分配车位的等待车辆，只记录车牌和到达时间。
7. `TransactionRecord` 保存车辆出场后的历史信息，包含车牌、位置、时间和费用。
8. `FeeRule` 被出场结算功能依赖，用于计算交易费用。
9. 统计数据依赖 `history`、`spots` 和 `total_revenue` 派生，不一定单独持久化。

E-R 图如下：

![停车场系统数据对象 E-R 图](./停车场系统数据对象ER图.png)

## 六、各数据对象上的基本操作和函数原型

### 1. 位置对象操作

```c
SpotLocation make_loc(int f, int r, int c);
int loc_equal(SpotLocation a, SpotLocation b);
```

说明：

- `make_loc` 创建一个车位坐标。
- `loc_equal` 判断两个车位坐标是否相同。

### 2. 系统初始化和销毁操作

```c
void ds_init(ParkingSystem *ps);
void ds_destroy(ParkingSystem *ps);
```

说明：

- `ds_init` 初始化车位数组、空闲栈、等待队列、哈希表、历史记录和计费规则。
- `ds_destroy` 释放动态内存，包括空闲栈、等待队列、哈希表和历史数组。

### 3. 空闲车位栈操作

```c
void stack_init(FreeSpotStack *s, int capacity);
void stack_destroy(FreeSpotStack *s);
void stack_push(FreeSpotStack *s, SpotLocation loc);
SpotLocation stack_pop(FreeSpotStack *s);
int stack_is_empty(const FreeSpotStack *s);
int stack_size(const FreeSpotStack *s);
```

说明：

- 支持车位快速分配和释放。
- `stack_pop` 用于入场分配空位。
- `stack_push` 用于释放空位。

### 4. 等待队列操作

```c
void queue_init(WaitingQueue *q);
void queue_clear(WaitingQueue *q);
void queue_enqueue(WaitingQueue *q, const char *plate, time_t t);
int queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t);
int queue_is_empty(const WaitingQueue *q);
int queue_count(const WaitingQueue *q);
```

说明：

- `queue_enqueue` 在停车场满位时加入等待车辆。
- `queue_dequeue` 在有车位释放时取出队头车辆。
- `queue_count` 用于显示当前排队车辆数量。

### 5. 哈希表操作

```c
unsigned int hash_str(const char *str);
void ht_init(HashMap *ht);
void ht_insert(HashMap *ht, VehicleRecord *rec);
VehicleRecord *ht_find(HashMap *ht, const char *plate);
void ht_remove(HashMap *ht, const char *plate);
void ht_clear(HashMap *ht);
```

说明：

- 哈希表以车牌号为关键字。
- `ht_find` 支持实时查找车辆。
- `ht_insert` 在车辆入场成功后插入索引。
- `ht_remove` 在车辆出场后删除索引。

### 6. 停车业务操作

```c
EntryResult vehicle_entry(ParkingSystem *ps, const char *plate);
ExitResult vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee);
VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate);
int find_in_queue(ParkingSystem *ps, const char *plate);
```

说明：

- `vehicle_entry` 完成入场、分配车位或进入等待队列。
- `vehicle_exit` 完成出场、计费、释放车位和自动调度。
- `find_vehicle` 查询在场车辆。
- `find_in_queue` 查询等待队列中的车辆位置。

### 7. 计费操作

```c
double calculate_fee(const FeeRule *rule, int duration_minutes);
void set_fee_rule(ParkingSystem *ps, FeeRule rule);
```

说明：

- `calculate_fee` 根据停车时长和计费规则计算费用。
- `set_fee_rule` 修改系统计费规则。

### 8. 统计分析操作

统计模块由 zxc 负责，主要基于 `history`、`spots` 和 `total_revenue` 派生统计结果，可设计如下函数原型：

```c
double stats_total_revenue(const ParkingSystem *ps);
int stats_floor_used_count(const ParkingSystem *ps, int floor);
double stats_floor_usage_rate(const ParkingSystem *ps, int floor);
void stats_peak_hours(const ParkingSystem *ps, int hour_counts[24]);
void stats_print_transactions(const ParkingSystem *ps);
```

说明：

- 统计收入依赖 `total_revenue` 和 `history`。
- 楼层热度依赖 `spots` 的占用状态。
- 高峰时段依赖历史交易的入场时间或出场时间。

### 9. 数据持久化操作

文件持久化模块由 zxc 负责，可设计如下函数原型：

```c
int save_data(ParkingSystem *ps, const char *filename);
int load_data(ParkingSystem *ps, const char *filename);
void rebuild_hash_from_spots(ParkingSystem *ps);
```

说明：

- `save_data` 保存车位、等待队列、历史交易、收入和计费规则。
- `load_data` 从文件恢复系统状态。
- `rebuild_hash_from_spots` 在加载后重建车牌哈希索引。

## 七、总结

本系统的数据对象设计以 `ParkingSystem` 为核心，通过多维数组、顺序栈、链式队列、哈希表和动态顺序表共同支撑停车场业务。

- 车位数组解决多层车位管理问题。
- 空闲车位栈解决 O(1) 车位分配和释放问题。
- 等待队列解决满位排队问题。
- 哈希表解决车牌实时查找问题。
- 计费规则和历史交易记录解决出场收费和统计分析问题。
- 系统全局状态对象统一组织各类数据，便于业务模块、统计模块、界面模块和持久化模块协同工作。

因此，该数据对象设计能够较好地满足立体多层停车场管理系统的业务需求和功能实现需要。
