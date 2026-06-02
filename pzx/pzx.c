/**
 * pzx.c — 数据结构层实现
 *
 * 功能：栈、队列、哈希表、系统初始化/销毁
 * 对应分工文档中的 ds.h + ds.c
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* =========================================================
 *                    常量定义
 * ========================================================= */
#define FLOORS      3
#define ROWS        4
#define COLS        5
#define TOTAL_SPOTS (FLOORS * ROWS * COLS)
#define HT_SIZE     10007

/* =========================================================
 *                    枚举定义
 * ========================================================= */
typedef enum { SPOT_FREE, SPOT_OCCUPIED } SpotStatus;
typedef enum { ENTRY_SUCCESS, PARKING_FULL, PLATE_EXISTS } EntryResult;
typedef enum { EXIT_SUCCESS, NOT_FOUND, NOT_PARKED } ExitResult;

/* =========================================================
 *                   核心结构体定义
 * ========================================================= */
/* 3D 坐标 */
typedef struct { int floor, row, col; } SpotLocation;

/* 单个车位 */
typedef struct {
    SpotLocation loc;
    SpotStatus status;
    char plate[16];
} ParkingSpot;

/* 车辆在场记录（存哈希表中） */
typedef struct {
    char plate[16];
    SpotLocation loc;
    time_t entry_time;
} VehicleRecord;

/* 空闲车位栈（O(1)分配核心） */
typedef struct {
    SpotLocation *spots;
    int top;
    int capacity;
} FreeSpotStack;

/* 等待队列节点 */
typedef struct QueueNode {
    char plate[16];
    time_t arrival_time;
    struct QueueNode *next;
} QueueNode;

/* 等待队列 */
typedef struct {
    QueueNode *front, *rear;
    int count;
} WaitingQueue;

/* 哈希表节点 */
typedef struct HTNode {
    char plate[16];
    VehicleRecord *record;
    struct HTNode *next;
} HTNode;

/* 哈希表 */
typedef struct {
    HTNode *buckets[HT_SIZE];
    int count;
} HashMap;

/* 计费规则 */
typedef struct {
    double base_fee;
    int free_minutes;
    double rate_per_hour;
    double max_daily;
} FeeRule;

/* 历史交易记录 */
typedef struct {
    char plate[16];
    SpotLocation loc;
    time_t entry_time;
    time_t exit_time;
    int duration_minutes;
    double fee;
} TransactionRecord;

/* 全局系统状态 */
typedef struct {
    ParkingSpot spots[FLOORS][ROWS][COLS];
    HashMap plate_index;
    FreeSpotStack free_spots;
    WaitingQueue wait_queue;
    TransactionRecord *history;
    int history_count;
    int history_capacity;
    FeeRule fee_rule;
    double total_revenue;
} ParkingSystem;

/* =========================================================
 *                    函数声明
 * ========================================================= */
/* ---- 辅助函数 ---- */
SpotLocation make_loc(int f, int r, int c);
int loc_equal(SpotLocation a, SpotLocation b);

/* ---- 哈希函数 ---- */
unsigned int hash_str(const char *str);

/* ---- 空闲车位栈 ---- */
void stack_init(FreeSpotStack *s, int capacity);
void stack_push(FreeSpotStack *s, SpotLocation loc);
SpotLocation stack_pop(FreeSpotStack *s);
int stack_is_empty(FreeSpotStack *s);
int stack_size(FreeSpotStack *s);

/* ---- 等待队列 ---- */
void queue_init(WaitingQueue *q);
void queue_enqueue(WaitingQueue *q, const char *plate, time_t t);
int  queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t);
int  queue_is_empty(WaitingQueue *q);
int  queue_count(WaitingQueue *q);

/* ---- 哈希表 ---- */
void ht_init(HashMap *ht);
void ht_insert(HashMap *ht, VehicleRecord *rec);
VehicleRecord *ht_find(HashMap *ht, const char *plate);
void ht_remove(HashMap *ht, const char *plate);
void ht_clear(HashMap *ht);

/* ---- 系统初始化与销毁 ---- */
void ds_init(ParkingSystem *ps);
void ds_destroy(ParkingSystem *ps);

/* =========================================================
 *                    辅助函数实现
 * ========================================================= */
SpotLocation make_loc(int f, int r, int c) {
    SpotLocation loc;
    loc.floor = f;
    loc.row   = r;
    loc.col   = c;
    return loc;
}

int loc_equal(SpotLocation a, SpotLocation b) {
    return a.floor == b.floor && a.row == b.row && a.col == b.col;
}

/* =========================================================
 *                    哈希函数
 * =========================================================
 * 算法：hash = hash * 31 + c
 * 31 可被编译器优化为 (hash << 5) - hash，效率高
 */
unsigned int hash_str(const char *str) {
    unsigned long hash = 0;
    while (*str) {
        hash = hash * 31 + (unsigned char)(*str++);
    }
    return (unsigned int)(hash % HT_SIZE);
}

/* =========================================================
 *                   空闲车位栈实现
 * ========================================================= */
void stack_init(FreeSpotStack *s, int capacity) {
    s->spots = (SpotLocation *)malloc(sizeof(SpotLocation) * capacity);
    if (s->spots == NULL) {
        printf("错误：内存分配失败（空闲栈）\n");
        exit(1);
    }
    s->top      = -1;
    s->capacity = capacity;
}

void stack_push(FreeSpotStack *s, SpotLocation loc) {
    if (s->top + 1 >= s->capacity) {
        s->capacity *= 2;
        SpotLocation *new_spots = (SpotLocation *)realloc(
            s->spots, sizeof(SpotLocation) * s->capacity);
        if (new_spots == NULL) {
            printf("错误：栈扩容失败\n");
            exit(1);
        }
        s->spots = new_spots;
    }
    s->spots[++s->top] = loc;
}

SpotLocation stack_pop(FreeSpotStack *s) {
    if (stack_is_empty(s)) {
        printf("错误：空闲栈已空，无法弹出\n");
        abort();
    }
    return s->spots[s->top--];
}

int stack_is_empty(FreeSpotStack *s) {
    return s->top == -1;
}

int stack_size(FreeSpotStack *s) {
    return s->top + 1;
}

/* =========================================================
 *                    等待队列实现
 * ========================================================= */
void queue_init(WaitingQueue *q) {
    q->front = q->rear = NULL;
    q->count = 0;
}

void queue_enqueue(WaitingQueue *q, const char *plate, time_t t) {
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    if (node == NULL) {
        printf("错误：内存分配失败（队列节点）\n");
        exit(1);
    }
    strcpy(node->plate, plate);
    node->arrival_time = t;
    node->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->count++;
}

int queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t) {
    if (queue_is_empty(q)) {
        return 0;
    }

    QueueNode *temp = q->front;
    strcpy(out_plate, temp->plate);
    *out_t = temp->arrival_time;

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    q->count--;
    return 1;
}

int queue_is_empty(WaitingQueue *q) {
    return q->front == NULL || q->count == 0;
}

int queue_count(WaitingQueue *q) {
    return q->count;
}

/* =========================================================
 *                    哈希表实现
 * ========================================================= */
void ht_init(HashMap *ht) {
    for (int i = 0; i < HT_SIZE; i++) {
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}

void ht_insert(HashMap *ht, VehicleRecord *rec) {
    unsigned int idx = hash_str(rec->plate);

    HTNode *node = (HTNode *)malloc(sizeof(HTNode));
    if (node == NULL) {
        printf("错误：内存分配失败（哈希表节点）\n");
        exit(1);
    }
    strcpy(node->plate, rec->plate);
    node->record = rec;

    /* 头插法 */
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->count++;
}

VehicleRecord *ht_find(HashMap *ht, const char *plate) {
    unsigned int idx = hash_str(plate);
    HTNode *p = ht->buckets[idx];
    while (p != NULL) {
        if (strcmp(p->plate, plate) == 0) {
            return p->record;
        }
        p = p->next;
    }
    return NULL;
}

void ht_remove(HashMap *ht, const char *plate) {
    unsigned int idx = hash_str(plate);
    HTNode *p   = ht->buckets[idx];
    HTNode *prev = NULL;

    while (p != NULL) {
        if (strcmp(p->plate, plate) == 0) {
            if (prev == NULL) {
                ht->buckets[idx] = p->next;
            } else {
                prev->next = p->next;
            }
            free(p);   /* 只释放 HTNode，不释放 VehicleRecord */
            ht->count--;
            return;
        }
        prev = p;
        p = p->next;
    }
}

void ht_clear(HashMap *ht) {
    for (int i = 0; i < HT_SIZE; i++) {
        HTNode *p = ht->buckets[i];
        while (p != NULL) {
            HTNode *temp = p;
            p = p->next;
            free(temp);
        }
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}

/* =========================================================
 *                 系统初始化与销毁
 * ========================================================= */
void ds_init(ParkingSystem *ps) {
    if (ps == NULL) return;

    /* 1. 初始化所有车位 — 空闲、无车牌 */
    for (int f = 0; f < FLOORS; f++) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                ps->spots[f][r][c].loc    = make_loc(f, r, c);
                ps->spots[f][r][c].status = SPOT_FREE;
                ps->spots[f][r][c].plate[0] = '\0';
            }
        }
    }

    /* 2. 初始化空闲栈，所有 60 个车位坐标入栈 */
    stack_init(&ps->free_spots, TOTAL_SPOTS);
    for (int f = 0; f < FLOORS; f++) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                stack_push(&ps->free_spots, make_loc(f, r, c));
            }
        }
    }

    /* 3. 初始化哈希表 */
    ht_init(&ps->plate_index);

    /* 4. 初始化等待队列 */
    queue_init(&ps->wait_queue);

    /* 5. 初始化历史记录动态数组（初始容量 100） */
    ps->history_capacity = 100;
    ps->history = (TransactionRecord *)malloc(
        sizeof(TransactionRecord) * ps->history_capacity);
    if (ps->history == NULL) {
        printf("错误：内存分配失败（历史记录）\n");
        exit(1);
    }
    ps->history_count = 0;

    /* 6. 设置默认计费规则 */
    ps->fee_rule.base_fee      = 5.0;
    ps->fee_rule.free_minutes  = 15;
    ps->fee_rule.rate_per_hour = 4.0;
    ps->fee_rule.max_daily     = 40.0;

    /* 7. 总收入归零 */
    ps->total_revenue = 0.0;
}

void ds_destroy(ParkingSystem *ps) {
    if (ps == NULL) return;

    /* 1. 释放空闲栈动态数组 */
    free(ps->free_spots.spots);
    ps->free_spots.spots     = NULL;
    ps->free_spots.top       = -1;
    ps->free_spots.capacity  = 0;

    /* 2. 清空哈希表（释放所有 HTNode，但 VehicleRecord 由 yjk 释放） */
    ht_clear(&ps->plate_index);

    /* 3. 释放等待队列所有剩余节点 */
    while (!queue_is_empty(&ps->wait_queue)) {
        char plate[16];
        time_t t;
        queue_dequeue(&ps->wait_queue, plate, &t);
    }

    /* 4. 释放历史记录数组 */
    free(ps->history);
    ps->history          = NULL;
    ps->history_count    = 0;
    ps->history_capacity = 0;
}
