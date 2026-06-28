#ifndef PARKING_H
#define PARKING_H

#include <time.h>

/* ═══════════════════════════════════════════
   常量定义
   ═══════════════════════════════════════════ */

#define FLOORS      3
#define ROWS        4
#define COLS        5
#define TOTAL_SPOTS (FLOORS * ROWS * COLS)
#define HT_SIZE     10007

/* ═══════════════════════════════════════════
   枚举类型
   ═══════════════════════════════════════════ */

typedef enum {
    SPOT_FREE,
    SPOT_OCCUPIED
} SpotStatus;

typedef enum {
    ENTRY_SUCCESS,
    PARKING_FULL,
    PLATE_EXISTS
} EntryResult;

typedef enum {
    PARK_EXIT_SUCCESS,
    NOT_FOUND,
    NOT_PARKED
} ExitResult;

typedef enum {
    USER_ADMIN,
    USER_OPERATOR,
    USER_CUSTOMER
} UserRole;

/* ═══════════════════════════════════════════
   基础数据结构
   ═══════════════════════════════════════════ */

typedef struct {
    int floor;
    int row;
    int col;
} SpotLocation;

typedef struct {
    SpotLocation loc;
    SpotStatus   status;
    char         plate[16];
    time_t       entry_time;   /* 入场时间，随车位数据持久化 */
} ParkingSpot;

typedef struct {
    char         plate[16];
    SpotLocation loc;
    time_t       entry_time;
} VehicleRecord;

/* 顺序栈 — 空闲车位 */
typedef struct {
    SpotLocation *spots;
    int           top;
    int           capacity;
} FreeSpotStack;

/* 链式队列 — 等待车辆 */
typedef struct QueueNode {
    char             plate[16];
    time_t           arrival_time;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int        count;
} WaitingQueue;

/* 哈希表 — 车牌索引 */
typedef struct HTNode {
    char             plate[16];
    VehicleRecord   *record;
    struct HTNode   *next;
} HTNode;

typedef struct {
    HTNode *buckets[HT_SIZE];
    int     count;
} HashMap;

/* 计费规则 */
typedef struct {
    double base_fee;
    int    free_minutes;
    double rate_per_hour;
    double max_daily;
} FeeRule;

/* 交易记录 */
typedef struct {
    char         plate[16];
    SpotLocation loc;
    time_t       entry_time;
    time_t       exit_time;
    int          duration_minutes;
    double       fee;
} TransactionRecord;

/* 用户账号 */
typedef struct {
    char     username[32];
    char     password[32];
    UserRole role;
    char     plate[16];
} User;

/* 用户数据库 */
typedef struct {
    User *users;
    int   count;
    int   capacity;
} UserDatabase;

/* ═══════════════════════════════════════════
   全局上下文
   ═══════════════════════════════════════════ */

typedef struct {
    ParkingSpot        spots[FLOORS][ROWS][COLS];
    HashMap            plate_index;
    FreeSpotStack      free_spots;
    WaitingQueue       wait_queue;
    TransactionRecord *history;
    int                history_count;
    int                history_capacity;
    FeeRule            fee_rule;
    double             total_revenue;
    UserDatabase       user_db;
} ParkingSystem;

/* ═══════════════════════════════════════════
   文件 I/O 结构
   ═══════════════════════════════════════════ */

#define FILE_MAGIC   0x504B1204
#define FILE_VERSION 1

typedef struct {
    int     magic;
    int     version;
    int     history_count;
    int     free_count;
    int     queue_count;
    double  total_revenue;
    FeeRule fee_rule;
} FileHeader;

/* ═══════════════════════════════════════════
   函数声明 — 基础数据结构
   ═══════════════════════════════════════════ */

SpotLocation make_loc(int f, int r, int c);
int          loc_equal(SpotLocation a, SpotLocation b);

void         stack_init(FreeSpotStack *s, int capacity);
void         stack_destroy(FreeSpotStack *s);
void         stack_push(FreeSpotStack *s, SpotLocation loc);
SpotLocation stack_pop(FreeSpotStack *s);
SpotLocation stack_pop_random(FreeSpotStack *s);
int          stack_is_empty(const FreeSpotStack *s);
int          stack_size(const FreeSpotStack *s);

void         queue_init(WaitingQueue *q);
void         queue_clear(WaitingQueue *q);
void         queue_enqueue(WaitingQueue *q, const char *plate, time_t t);
int          queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t);
int          queue_is_empty(const WaitingQueue *q);
int          queue_count(const WaitingQueue *q);

unsigned int    hash_str(const char *str);
void            ht_init(HashMap *ht);
void            ht_insert(HashMap *ht, VehicleRecord *rec);
VehicleRecord  *ht_find(HashMap *ht, const char *plate);
void            ht_remove(HashMap *ht, const char *plate);
void            ht_clear(HashMap *ht);

void ds_init(ParkingSystem *ps);
void ds_destroy(ParkingSystem *ps);

/* 用户数据库 */
void user_db_init(UserDatabase *db);
void user_db_destroy(UserDatabase *db);
int  user_db_add(UserDatabase *db, const char *username, const char *password,
                 UserRole role, const char *plate);
int  user_db_delete(UserDatabase *db, const char *username);
int  user_db_find(UserDatabase *db, const char *username, const char *password,
                  UserRole *out_role);
int  user_db_exists(UserDatabase *db, const char *username);

/* ═══════════════════════════════════════════
   函数声明 — 业务逻辑
   ═══════════════════════════════════════════ */

EntryResult    vehicle_entry(ParkingSystem *ps, const char *plate);
ExitResult     vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee);
VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate);
int            find_in_queue(ParkingSystem *ps, const char *plate);

double calculate_fee(const FeeRule *rule, int duration_minutes);
void   set_fee_rule(ParkingSystem *ps, FeeRule rule);

/* ═══════════════════════════════════════════
   函数声明 — 统计
   ═══════════════════════════════════════════ */

void stats_occupancy(ParkingSystem *ps);
void stats_revenue(ParkingSystem *ps, int mode);
void stats_transactions(ParkingSystem *ps);
void stats_plate_history(ParkingSystem *ps, const char *plate);
void stats_floor_usage(ParkingSystem *ps);
void stats_peak_hours(ParkingSystem *ps);

/* ═══════════════════════════════════════════
   函数声明 — UI
   ═══════════════════════════════════════════ */

int  ui_login(UserDatabase *db, UserRole *out_role, char out_username[32]);
void ui_user_manage(UserDatabase *db);
void ui_admin_menu(ParkingSystem *ps);
void ui_operator_menu(ParkingSystem *ps);
void ui_customer_menu(ParkingSystem *ps, const char *my_plate);

void ui_entry(ParkingSystem *ps);
void ui_exit(ParkingSystem *ps);
void ui_find(ParkingSystem *ps);
void ui_free_spots(ParkingSystem *ps);
void ui_wait_queue(ParkingSystem *ps);
void ui_parking_map(ParkingSystem *ps);
void ui_revenue(ParkingSystem *ps);
void ui_transactions(ParkingSystem *ps);
void ui_floor_usage(ParkingSystem *ps);
void ui_peak_hours(ParkingSystem *ps);
void ui_save(ParkingSystem *ps);
void ui_load(ParkingSystem *ps);

/* ═══════════════════════════════════════════
   函数声明 — 文件 I/O
   ═══════════════════════════════════════════ */

int  save_data(ParkingSystem *ps, const char *filename);
int  load_data(ParkingSystem *ps, const char *filename);
void rebuild_hash_from_spots(ParkingSystem *ps);

int  save_users(UserDatabase *db, const char *filename);
int  load_users(UserDatabase *db, const char *filename);

#endif
