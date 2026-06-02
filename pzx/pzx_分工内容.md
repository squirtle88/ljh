# pzx — 数据结构层分工内容

> 提取自 `停车场管理系统_三人分工方案.md`

---

## 一、负责文件

- **ds.h** — 数据结构定义 + 函数声明
- **ds.c** — 栈/队列/哈希表实现

---

## 二、核心结构体定义 (ds.h)

```c
// ========== 车位状态 ==========
typedef enum { SPOT_FREE, SPOT_OCCUPIED } SpotStatus;
typedef enum { ENTRY_SUCCESS, PARKING_FULL, PLATE_EXISTS } EntryResult;
typedef enum { EXIT_SUCCESS, NOT_FOUND, NOT_PARKED } ExitResult;

// ========== 3D 坐标 ==========
typedef struct { int floor, row, col; } SpotLocation;

// ========== 车位 ==========
typedef struct {
    SpotLocation loc;
    SpotStatus status;
    char plate[16];
} ParkingSpot;

// ========== 车辆在场记录 ==========
typedef struct {
    char plate[16];
    SpotLocation loc;
    time_t entry_time;
} VehicleRecord;

// ========== 空闲车位栈（O(1)分配核心） ==========
typedef struct {
    SpotLocation *spots;
    int top;
    int capacity;
} FreeSpotStack;

// ========== 等待队列（FIFO） ==========
typedef struct QueueNode {
    char plate[16];
    time_t arrival_time;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front, *rear;
    int count;
} WaitingQueue;

// ========== 哈希表 ==========
#define HT_SIZE 10007

typedef struct HTNode {
    char plate[16];
    VehicleRecord *record;
    struct HTNode *next;
} HTNode;

typedef struct {
    HTNode *buckets[HT_SIZE];
    int count;
} HashMap;

// ========== 计费规则 ==========
typedef struct {
    double base_fee;
    int free_minutes;
    double rate_per_hour;
    double max_daily;
} FeeRule;

// ========== 历史交易 ==========
typedef struct {
    char plate[16];
    SpotLocation loc;
    time_t entry_time;
    time_t exit_time;
    int duration_minutes;
    double fee;
} TransactionRecord;

// ========== 全局系统状态 ==========
typedef struct {
    ParkingSpot spots[3][4][5];
    HashMap plate_index;
    FreeSpotStack free_spots;
    WaitingQueue wait_queue;
    TransactionRecord *history;
    int history_count;
    int history_capacity;
    FeeRule fee_rule;
    double total_revenue;
} ParkingSystem;
```

---

## 三、pzx 需要实现的全部函数

```c
// ---- 初始化与销毁 ----
void ds_init(ParkingSystem *ps);
    /* 功能：初始化停车场系统
     * 1. 所有车位状态设为 FREE，plate 置空
     * 2. 初始化空闲栈，将所有车位坐标依次入栈
     * 3. 初始化哈希表（所有桶置 NULL）
     * 4. 初始化等待队列（front=rear=NULL）
     * 5. 初始化历史记录动态数组（初始容量 100）
     * 6. 设置默认计费规则：
     *      base_fee=5.0, free_minutes=15,
     *      rate_per_hour=4.0, max_daily=40.0
     * 7. total_revenue = 0.0
     */

void ds_destroy(ParkingSystem *ps);
    /* 功能：释放所有动态内存
     * 1. free(空闲栈.spots)
     * 2. 遍历哈希表每个桶，释放所有节点
     * 3. 释放等待队列所有节点
     * 4. free(history)
     */

// ---- 空闲车位栈 ----
void stack_init(FreeSpotStack *s, int capacity);
    /* 功能：初始化栈，分配 capacity 个 SpotLocation 空间
     * 实现：s->spots = malloc(...), s->top = -1
     */

void stack_push(FreeSpotStack *s, SpotLocation loc);
    /* 功能：将车位坐标入栈
     * 实现：s->spots[++s->top] = loc
     * 注意：如果 top+1 >= capacity，realloc 扩容为 2 倍
     */

SpotLocation stack_pop(FreeSpotStack *s);
    /* 功能：弹出栈顶车位坐标
     * 实现：return s->spots[s->top--]
     * 前置条件：!stack_is_empty(s)，否则 abort
     */

int stack_is_empty(FreeSpotStack *s);
    /* 功能：检查栈是否为空
     * 返回：s->top == -1
     */

int stack_size(FreeSpotStack *s);
    /* 功能：返回当前空闲车位数
     * 返回：s->top + 1
     */

// ---- 等待队列 ----
void queue_init(WaitingQueue *q);
    /* 功能：初始化空队列
     * 实现：q->front = q->rear = NULL; q->count = 0;
     */

void queue_enqueue(WaitingQueue *q, const char *plate, time_t t);
    /* 功能：将车辆加入队尾
     * 实现：创建 QueueNode，strcpy 车牌，记录时间
     *       如果队列空 → front = rear = newNode
     *       否则 → rear->next = newNode; rear = newNode
     *       count++
     */

int queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t);
    /* 功能：从队首取出一辆车
     * 返回：1=成功取出，0=队列空
     * 实现：保存 front 节点，strcpy 车牌和时间
     *       front = front->next，free 旧节点
     *       如果队列变空 → rear = NULL
     *       count--
     */

int queue_is_empty(WaitingQueue *q);
    /* 功能：判断队列是否为空
     * 返回：q->front == NULL 或 q->count == 0
     */

int queue_count(WaitingQueue *q);
    /* 功能：返回排队车辆数
     * 返回：q->count
     */

// ---- 哈希表 ----
unsigned int hash_str(const char *str);
    /* 功能：计算字符串哈希值
     * 算法：unsigned long h = 0;
     *       while (*str) h = h * 31 + (unsigned char)(*str++);
     *       return h % HT_SIZE;
     * 原理：31 是常用质数乘子，分布均匀
     */

void ht_init(HashMap *ht);
    /* 功能：初始化哈希表
     * 实现：for i=0..HT_SIZE-1: ht->buckets[i] = NULL
     *       ht->count = 0
     */

void ht_insert(HashMap *ht, VehicleRecord *rec);
    /* 功能：插入车辆记录
     * 实现：计算 hash = hash_str(rec->plate)
     *       创建 HTNode，strcpy 车牌，指向 record
     *       头插法插入 buckets[hash]
     *       ht->count++
     * 注意：不需要检查重复（调用方保证不重复）
     */

VehicleRecord *ht_find(HashMap *ht, const char *plate);
    /* 功能：通过车牌查找车辆在场记录
     * 实现：hash = hash_str(plate)
     *       遍历 buckets[hash] 链表，strcmp 匹配
     *       找到 → 返回 HTNode->record
     *       未找到 → return NULL
     */

void ht_remove(HashMap *ht, const char *plate);
    /* 功能：删除车辆记录（出场时调用）
     * 实现：hash = hash_str(plate)
     *       遍历链表，找到匹配节点
     *       修改前驱节点的 next 跳过该节点
     *       free HTNode（注意：只释放 HTNode，不释放 VehicleRecord）
     *       VehicleRecord 由调用方负责释放
     *       ht->count--
     * 注意：未找到时什么都不做
     */

void ht_clear(HashMap *ht);
    /* 功能：清空哈希表（销毁时调用）
     * 实现：遍历每个桶，free 所有 HTNode
     */

// ---- 辅助函数 ----
SpotLocation make_loc(int f, int r, int c);
    /* 功能：创建一个 SpotLocation 结构体 */

int loc_equal(SpotLocation a, SpotLocation b);
    /* 功能：判断两个坐标是否相等 */
```

---

## 四、关键算法：O(1) 车位分配/释放（空闲车位栈）

### 核心思想

```
初始化时：将所有车位坐标依次入栈
           ┌──────┬──────┬──────┬──────┬──────┐
  栈顶 →  │0,0,0 │0,0,1 │0,0,2 │ ...  │2,3,4 │  ← 栈底
           └──────┴──────┴──────┴──────┴──────┘

车辆入场：pop() → 取出一个空闲坐标 → O(1)
车辆出场：push(该车位坐标) → 放回栈顶 → O(1)
```

### ds_init 中初始化空闲栈的关键代码

```c
// 初始化空闲车位栈：将所有车位坐标入栈
stack_init(&ps->free_spots, TOTAL_SPOTS);
for (int f = 0; f < FLOORS; f++) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            stack_push(&ps->free_spots, make_loc(f, r, c));
            ps->spots[f][r][c].loc = make_loc(f, r, c);
            ps->spots[f][r][c].status = SPOT_FREE;
            ps->spots[f][r][c].plate[0] = '\0';
        }
    }
}
```

### 哈希函数

```c
unsigned int hash_str(const char *str) {
    unsigned long hash = 0;
    while (*str) {
        hash = hash * 31 + (unsigned char)(*str++);
    }
    return hash % HT_SIZE;
}
```

选择 31 的原因：乘法可被编译器优化为 `(hash << 5) - hash`，效率高。

---

## 五、pzx 提供给 yjk 和 zxc 的接口

| 函数 | 功能 | 被谁调用 |
|------|------|---------|
| `ds_init(ps)` | 初始化系统 | yjk, zxc (main) |
| `ds_destroy(ps)` | 销毁系统 | zxc (main) |
| `stack_push(s, loc)` | 空闲车位入栈 | yjk |
| `stack_pop(s)` | 分配空闲车位 | yjk |
| `stack_is_empty(s)` | 检查是否有空位 | yjk, zxc |
| `stack_size(s)` | 空闲车位数 | zxc |
| `queue_enqueue(q, plate, t)` | 加入排队 | yjk |
| `queue_dequeue(q, out_plate, out_t)` | 排队出队 | yjk |
| `queue_is_empty(q)` | 检查排队是否空 | yjk |
| `queue_count(q)` | 排队人数 | zxc |
| `ht_insert(ht, rec)` | 哈希表插入 | yjk |
| `ht_find(ht, plate)` | 哈希表查找 | yjk, zxc |
| `ht_remove(ht, plate)` | 哈希表删除 | yjk |
| `make_loc(f, r, c)` | 创建坐标 | yjk, zxc |

---

## 六、开发任务

| 轮次 | 任务 |
|------|------|
| 第一轮 | 完成 `ds.h` 和 `ds.c` —— 所有结构体定义 + 栈/队列/哈希表全部实现 |
| 第二轮 | 自测 ds.c 所有函数（写测试主函数验证栈/队列/哈希表） |

---

## 七、验收项

| # | 功能 | 说明 |
|---|------|------|
| 1 | 系统初始化 | 创建 60 个空闲车位 |
| 2 | 空闲栈 | pop/push 操作正确 |
| 3 | 哈希表 | 插入/查找/删除 O(1) |
| 4 | 等待队列 | 入队/出队 FIFO |
| 17 | 内存管理 | 无内存泄漏（pzx 负责 ds 部分） |

---

## 八、代码规范

| 类别 | 规范 | 示例 |
|------|------|------|
| 结构体 | 大驼峰 | `VehicleRecord`, `ParkingSpot` |
| 枚举 | 大写下划线 | `SPOT_FREE`, `ENTRY_SUCCESS` |
| 函数 | 小写下划线 | `vehicle_entry`, `ht_find` |
| 变量 | 小写下划线 | `free_count`, `entry_time` |
| 宏 | 大写下划线 | `TOTAL_SPOTS`, `HT_SIZE` |
| 类型 | `typedef` + 大驼峰 | `ParkingSystem`, `HashMap` |
