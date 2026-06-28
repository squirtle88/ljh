#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "parking.h"

/* ═══════════════════════════════════════════
   内部工具函数
   ═══════════════════════════════════════════ */

static void copy_plate(char dest[16], const char *src) {
    if (src == NULL) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, 15);
    dest[15] = '\0';
}

static int is_valid_plate(const char *plate) {
    if (plate == NULL) return 0;
    size_t len = strlen(plate);
    return len > 0 && len < sizeof(((VehicleRecord *)0)->plate);
}

/* ═══════════════════════════════════════════
   SpotLocation 辅助函数
   ═══════════════════════════════════════════ */

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

/* ═══════════════════════════════════════════
   顺序栈 — 空闲车位管理
   ═══════════════════════════════════════════ */

void stack_init(FreeSpotStack *s, int capacity) {
    if (s == NULL || capacity <= 0) return;
    s->spots = (SpotLocation *)malloc(sizeof(SpotLocation) * capacity);
    if (s->spots == NULL) {
        fprintf(stderr, "stack_init: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    s->top      = -1;
    s->capacity = capacity;
}

void stack_destroy(FreeSpotStack *s) {
    if (s == NULL) return;
    free(s->spots);
    s->spots    = NULL;
    s->top      = -1;
    s->capacity = 0;
}

void stack_push(FreeSpotStack *s, SpotLocation loc) {
    if (s == NULL) return;
    if (s->top + 1 >= s->capacity) {
        int new_capacity = s->capacity > 0 ? s->capacity * 2 : 1;
        SpotLocation *new_spots = (SpotLocation *)realloc(
            s->spots, sizeof(SpotLocation) * new_capacity);
        if (new_spots == NULL) {
            fprintf(stderr, "stack_push: stack expansion failed\n");
            exit(EXIT_FAILURE);
        }
        s->spots    = new_spots;
        s->capacity = new_capacity;
    }
    s->spots[++s->top] = loc;
}

SpotLocation stack_pop(FreeSpotStack *s) {
    if (s == NULL || stack_is_empty(s)) {
        fprintf(stderr, "stack_pop: empty stack\n");
        abort();
    }
    return s->spots[s->top--];
}

SpotLocation stack_pop_random(FreeSpotStack *s) {
    if (s == NULL || stack_is_empty(s)) {
        fprintf(stderr, "stack_pop_random: empty stack\n");
        abort();
    }
    int idx = rand() % (s->top + 1);
    SpotLocation chosen = s->spots[idx];
    s->spots[idx] = s->spots[s->top];
    s->top--;
    return chosen;
}

int stack_is_empty(const FreeSpotStack *s) {
    return s == NULL || s->top == -1;
}

int stack_size(const FreeSpotStack *s) {
    if (s == NULL) return 0;
    return s->top + 1;
}

/* ═══════════════════════════════════════════
   链式队列 — 等待车辆管理
   ═══════════════════════════════════════════ */

void queue_init(WaitingQueue *q) {
    if (q == NULL) return;
    q->front = NULL;
    q->rear  = NULL;
    q->count = 0;
}

void queue_clear(WaitingQueue *q) {
    if (q == NULL) return;
    while (!queue_is_empty(q)) {
        char plate[16]; time_t t;
        queue_dequeue(q, plate, &t);
    }
}

void queue_enqueue(WaitingQueue *q, const char *plate, time_t t) {
    if (q == NULL) return;
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    if (node == NULL) {
        fprintf(stderr, "queue_enqueue: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    copy_plate(node->plate, plate);
    node->arrival_time = t;
    node->next = NULL;
    if (q->rear == NULL) {
        q->front = node;
        q->rear  = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->count++;
}

int queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t) {
    if (q == NULL || queue_is_empty(q)) return 0;
    QueueNode *old_front = q->front;
    if (out_plate != NULL) copy_plate(out_plate, old_front->plate);
    if (out_t != NULL) *out_t = old_front->arrival_time;
    q->front = old_front->next;
    if (q->front == NULL) q->rear = NULL;
    free(old_front);
    q->count--;
    return 1;
}

int queue_is_empty(const WaitingQueue *q) {
    return q == NULL || q->front == NULL || q->count == 0;
}

int queue_count(const WaitingQueue *q) {
    if (q == NULL) return 0;
    return q->count;
}

/* ═══════════════════════════════════════════
   哈希表 — 车牌索引
   ═══════════════════════════════════════════ */

unsigned int hash_str(const char *str) {
    unsigned long hash = 0;
    if (str == NULL) return 0;
    while (*str != '\0')
        hash = hash * 31 + (unsigned char)(*str++);
    return (unsigned int)(hash % HT_SIZE);
}

void ht_init(HashMap *ht) {
    if (ht == NULL) return;
    for (int i = 0; i < HT_SIZE; i++)
        ht->buckets[i] = NULL;
    ht->count = 0;
}

void ht_insert(HashMap *ht, VehicleRecord *rec) {
    if (ht == NULL || rec == NULL) return;
    unsigned int idx = hash_str(rec->plate);
    HTNode *node = (HTNode *)malloc(sizeof(HTNode));
    if (node == NULL) {
        fprintf(stderr, "ht_insert: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    copy_plate(node->plate, rec->plate);
    node->record = rec;
    node->next   = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->count++;
}

VehicleRecord *ht_find(HashMap *ht, const char *plate) {
    if (ht == NULL || plate == NULL) return NULL;
    unsigned int idx = hash_str(plate);
    HTNode *p = ht->buckets[idx];
    while (p != NULL) {
        if (strcmp(p->plate, plate) == 0) return p->record;
        p = p->next;
    }
    return NULL;
}

void ht_remove(HashMap *ht, const char *plate) {
    if (ht == NULL || plate == NULL) return;
    unsigned int idx = hash_str(plate);
    HTNode *p = ht->buckets[idx];
    HTNode *prev = NULL;
    while (p != NULL) {
        if (strcmp(p->plate, plate) == 0) {
            if (prev == NULL)
                ht->buckets[idx] = p->next;
            else
                prev->next = p->next;
            free(p);
            ht->count--;
            return;
        }
        prev = p;
        p = p->next;
    }
}

void ht_clear(HashMap *ht) {
    if (ht == NULL) return;
    for (int i = 0; i < HT_SIZE; i++) {
        HTNode *p = ht->buckets[i];
        while (p != NULL) {
            HTNode *next = p->next;
            free(p);
            p = next;
        }
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}

/* ═══════════════════════════════════════════
   系统初始化 / 销毁
   ═══════════════════════════════════════════ */

void ds_init(ParkingSystem *ps) {
    if (ps == NULL) return;
    srand((unsigned int)time(NULL));

    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                ps->spots[f][r][c].loc    = make_loc(f, r, c);
                ps->spots[f][r][c].status = SPOT_FREE;
                ps->spots[f][r][c].plate[0] = '\0';
                ps->spots[f][r][c].entry_time = 0;
            }

    stack_init(&ps->free_spots, TOTAL_SPOTS);
    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                stack_push(&ps->free_spots, make_loc(f, r, c));

    ht_init(&ps->plate_index);
    queue_init(&ps->wait_queue);

    ps->history_capacity = 100;
    ps->history = (TransactionRecord *)malloc(
        sizeof(TransactionRecord) * ps->history_capacity);
    if (ps->history == NULL) {
        fprintf(stderr, "ds_init: history allocation failed\n");
        exit(EXIT_FAILURE);
    }
    ps->history_count = 0;

    ps->fee_rule.base_fee     = 5.0;
    ps->fee_rule.free_minutes = 15;
    ps->fee_rule.rate_per_hour = 4.0;
    ps->fee_rule.max_daily    = 40.0;
    ps->total_revenue = 0.0;

    user_db_init(&ps->user_db);
}

void ds_destroy(ParkingSystem *ps) {
    if (ps == NULL) return;
    stack_destroy(&ps->free_spots);
    ht_clear(&ps->plate_index);
    queue_clear(&ps->wait_queue);
    free(ps->history);
    ps->history = NULL;
    ps->history_count = 0;
    ps->history_capacity = 0;
    user_db_destroy(&ps->user_db);
}

/* ═══════════════════════════════════════════
   用户数据库操作
   ═══════════════════════════════════════════ */

void user_db_init(UserDatabase *db) {
    if (db == NULL) return;
    db->capacity = 8;
    db->users = (User *)malloc(sizeof(User) * db->capacity);
    if (db->users == NULL) {
        fprintf(stderr, "user_db_init: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    db->count = 0;
}

void user_db_destroy(UserDatabase *db) {
    if (db == NULL) return;
    free(db->users);
    db->users    = NULL;
    db->count    = 0;
    db->capacity = 0;
}

int user_db_add(UserDatabase *db, const char *username, const char *password,
                UserRole role, const char *plate) {
    if (db == NULL || username == NULL || password == NULL) return 0;
    for (int i = 0; i < db->count; i++)
        if (strcmp(db->users[i].username, username) == 0) return 0;

    if (db->count >= db->capacity) {
        int new_cap = db->capacity * 2;
        User *new_users = (User *)realloc(db->users, sizeof(User) * new_cap);
        if (new_users == NULL) return 0;
        db->users = new_users;
        db->capacity = new_cap;
    }

    strncpy(db->users[db->count].username, username, 31);
    db->users[db->count].username[31] = '\0';
    strncpy(db->users[db->count].password, password, 31);
    db->users[db->count].password[31] = '\0';
    db->users[db->count].role = role;
    if (plate != NULL) {
        strncpy(db->users[db->count].plate, plate, 15);
        db->users[db->count].plate[15] = '\0';
    } else {
        db->users[db->count].plate[0] = '\0';
    }
    db->count++;
    return 1;
}

int user_db_delete(UserDatabase *db, const char *username) {
    if (db == NULL || username == NULL) return 0;
    int admin_count = 0, target_is_admin = 0;
    for (int i = 0; i < db->count; i++) {
        if (db->users[i].role == USER_ADMIN) {
            admin_count++;
            if (strcmp(db->users[i].username, username) == 0)
                target_is_admin = 1;
        }
    }
    if (target_is_admin && admin_count <= 1) return 0;

    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->users[i].username, username) == 0) {
            db->users[i] = db->users[db->count - 1];
            db->count--;
            return 1;
        }
    }
    return 0;
}

int user_db_find(UserDatabase *db, const char *username, const char *password,
                 UserRole *out_role) {
    if (db == NULL || username == NULL || password == NULL) return 0;
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->users[i].username, username) == 0 &&
            strcmp(db->users[i].password, password) == 0) {
            if (out_role != NULL) *out_role = db->users[i].role;
            return 1;
        }
    }
    return 0;
}

int user_db_exists(UserDatabase *db, const char *username) {
    if (db == NULL || username == NULL) return 0;
    for (int i = 0; i < db->count; i++)
        if (strcmp(db->users[i].username, username) == 0) return 1;
    return 0;
}

/* ═══════════════════════════════════════════
   计费模块
   ═══════════════════════════════════════════ */

double calculate_fee(const FeeRule *rule, int duration_minutes) {
    if (rule == NULL || duration_minutes <= rule->free_minutes)
        return 0.0;

    int charged_minutes = duration_minutes - rule->free_minutes;
    double charged_hours = ceil(charged_minutes / 60.0);
    double fee = rule->base_fee + charged_hours * rule->rate_per_hour;

    if (rule->max_daily > 0.0 && fee > rule->max_daily)
        fee = rule->max_daily;

    return fee;
}

void set_fee_rule(ParkingSystem *ps, FeeRule rule) {
    if (ps == NULL) return;
    ps->fee_rule = rule;
}

/* ═══════════════════════════════════════════
   业务逻辑 — 内部辅助
   ═══════════════════════════════════════════ */

static VehicleRecord *create_vehicle_record(const char *plate,
                                            SpotLocation loc,
                                            time_t entry_time) {
    VehicleRecord *rec = (VehicleRecord *)malloc(sizeof(VehicleRecord));
    if (rec == NULL) {
        fprintf(stderr, "create_vehicle_record: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    copy_plate(rec->plate, plate);
    rec->loc = loc;
    rec->entry_time = entry_time;
    return rec;
}

static void occupy_spot(ParkingSystem *ps, SpotLocation loc,
                        const char *plate, time_t entry_time) {
    VehicleRecord *rec = create_vehicle_record(plate, loc, entry_time);
    ps->spots[loc.floor][loc.row][loc.col].status = SPOT_OCCUPIED;
    copy_plate(ps->spots[loc.floor][loc.row][loc.col].plate, plate);
    ps->spots[loc.floor][loc.row][loc.col].entry_time = entry_time;
    ht_insert(&ps->plate_index, rec);
}

static void ensure_history_capacity(ParkingSystem *ps) {
    if (ps->history_count < ps->history_capacity) return;
    int new_capacity = ps->history_capacity > 0 ? ps->history_capacity * 2 : 100;
    TransactionRecord *new_history = (TransactionRecord *)realloc(
        ps->history, sizeof(TransactionRecord) * new_capacity);
    if (new_history == NULL) {
        fprintf(stderr, "ensure_history_capacity: history expansion failed\n");
        exit(EXIT_FAILURE);
    }
    ps->history = new_history;
    ps->history_capacity = new_capacity;
}

static void append_transaction(ParkingSystem *ps, const VehicleRecord *rec,
                               time_t exit_time, int duration_minutes,
                               double fee) {
    ensure_history_capacity(ps);
    TransactionRecord *record = &ps->history[ps->history_count++];
    copy_plate(record->plate, rec->plate);
    record->loc              = rec->loc;
    record->entry_time       = rec->entry_time;
    record->exit_time        = exit_time;
    record->duration_minutes = duration_minutes;
    record->fee              = fee;
}

/* ═══════════════════════════════════════════
   业务逻辑 — 入场 / 出场 / 查找
   ═══════════════════════════════════════════ */

EntryResult vehicle_entry(ParkingSystem *ps, const char *plate) {
    if (ps == NULL || !is_valid_plate(plate))
        return PLATE_EXISTS;

    if (ht_find(&ps->plate_index, plate) != NULL ||
        find_in_queue(ps, plate) != -1)
        return PLATE_EXISTS;

    if (stack_is_empty(&ps->free_spots)) {
        queue_enqueue(&ps->wait_queue, plate, time(NULL));
        return PARKING_FULL;
    }

    occupy_spot(ps, stack_pop_random(&ps->free_spots), plate, time(NULL));
    return ENTRY_SUCCESS;
}

ExitResult vehicle_exit(ParkingSystem *ps, const char *plate, double *out_fee) {
    if (out_fee != NULL) *out_fee = 0.0;
    if (ps == NULL || !is_valid_plate(plate)) return NOT_FOUND;

    VehicleRecord *rec = ht_find(&ps->plate_index, plate);
    if (rec == NULL)
        return find_in_queue(ps, plate) == -1 ? NOT_FOUND : NOT_PARKED;

    SpotLocation released_loc = rec->loc;
    time_t now = time(NULL);
    int duration_minutes = (int)(difftime(now, rec->entry_time) / 60.0);
    if (duration_minutes < 0) duration_minutes = 0;

    double fee = calculate_fee(&ps->fee_rule, duration_minutes);
    if (out_fee != NULL) *out_fee = fee;

    append_transaction(ps, rec, now, duration_minutes, fee);
    ps->total_revenue += fee;

    ps->spots[released_loc.floor][released_loc.row][released_loc.col].status = SPOT_FREE;
    ps->spots[released_loc.floor][released_loc.row][released_loc.col].plate[0] = '\0';
    ps->spots[released_loc.floor][released_loc.row][released_loc.col].entry_time = 0;

    ht_remove(&ps->plate_index, plate);
    free(rec);

    if (!queue_is_empty(&ps->wait_queue)) {
        char next_plate[16];
        time_t queue_arrival_time;
        if (queue_dequeue(&ps->wait_queue, next_plate, &queue_arrival_time)) {
            (void)queue_arrival_time;
            occupy_spot(ps, released_loc, next_plate, now);
        }
    } else {
        stack_push(&ps->free_spots, released_loc);
    }

    return PARK_EXIT_SUCCESS;
}

VehicleRecord *find_vehicle(ParkingSystem *ps, const char *plate) {
    if (ps == NULL || !is_valid_plate(plate)) return NULL;
    return ht_find(&ps->plate_index, plate);
}

int find_in_queue(ParkingSystem *ps, const char *plate) {
    if (ps == NULL || !is_valid_plate(plate)) return -1;
    QueueNode *p = ps->wait_queue.front;
    int pos = 1;
    while (p != NULL) {
        if (strcmp(p->plate, plate) == 0) return pos;
        p = p->next;
        pos++;
    }
    return -1;
}

/* ═══════════════════════════════════════════
   统计功能
   ═══════════════════════════════════════════ */

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
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED) used++;

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
        const char *title = "";
        if (mode == 1) title = "今日收入统计";
        else if (mode == 2) title = "本月收入统计";
        else if (mode == 3) title = "上周收入统计";

        printf("         %s\n", title);
        printf("═══════════════════════════════════════\n");

        if (mode == 1) {
            for (int i = 0; i < ps->history_count; i++) {
                struct tm *tm_entry = localtime(&ps->history[i].entry_time);
                if (tm_entry->tm_year == tm_now->tm_year &&
                    tm_entry->tm_mon  == tm_now->tm_mon  &&
                    tm_entry->tm_mday == tm_now->tm_mday) {
                    revenue += ps->history[i].fee; count++;
                }
            }
        } else if (mode == 2) {
            for (int i = 0; i < ps->history_count; i++) {
                struct tm *tm_entry = localtime(&ps->history[i].entry_time);
                if (tm_entry->tm_year == tm_now->tm_year &&
                    tm_entry->tm_mon  == tm_now->tm_mon) {
                    revenue += ps->history[i].fee; count++;
                }
            }
        } else if (mode == 3) {
            int days_since_monday = tm_now->tm_wday == 0 ? 6 : tm_now->tm_wday - 1;
            time_t last_monday = now - (days_since_monday + 7) * 86400;
            struct tm *tm_start = localtime(&last_monday);
            tm_start->tm_hour = 0; tm_start->tm_min = 0; tm_start->tm_sec = 0;
            time_t week_start = mktime(tm_start);
            time_t last_sunday = week_start + 7 * 86400 - 1;

            for (int i = 0; i < ps->history_count; i++) {
                if (ps->history[i].entry_time >= week_start &&
                    ps->history[i].entry_time <= last_sunday) {
                    revenue += ps->history[i].fee; count++;
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
    if (ps == NULL || ps->history_count == 0) {
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

    typedef struct { int floor; int count; double turnover; } FloorStat;
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

    /* 统计历史交易（已出场车辆）的入场时间 */
    for (int i = 0; i < ps->history_count; i++) {
        struct tm *tm_entry = localtime(&ps->history[i].entry_time);
        hour_count[tm_entry->tm_hour]++;
    }

    /* 统计当前在场车辆的入场时间 */
    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED) {
                    struct tm *tm_entry = localtime(&ps->spots[f][r][c].entry_time);
                    hour_count[tm_entry->tm_hour]++;
                }

    int top_hours[3] = {-1, -1, -1};
    for (int h = 0; h < 24; h++) {
        if (hour_count[h] == 0) continue;
        for (int j = 0; j < 3; j++) {
            if (top_hours[j] == -1 || hour_count[h] > hour_count[top_hours[j]]) {
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

/* ═══════════════════════════════════════════
   UI — 登录
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
        scanf("%31s", username); getchar();
        printf("  密  码: ");
        scanf("%31s", password); getchar();

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
   UI — 用户管理（管理员专用）
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
        scanf("%d", &choice); getchar();

        switch (choice) {
            case 1: {
                char name[32], pass[32], plate[16] = "";
                int role_choice;
                printf("  角色: 1.管理员 2.操作员 3.车主\n");
                printf("  选择: ");
                scanf("%d", &role_choice); getchar();
                if (role_choice < 1 || role_choice > 3) {
                    printf("  无效角色！\n"); break;
                }
                if (role_choice == 3) {
                    printf("  绑定车牌号: ");
                    scanf("%15s", plate); getchar();
                }
                printf("  输入新用户名: ");
                scanf("%31s", name); getchar();
                if (user_db_exists(db, name)) {
                    printf("  用户名已存在！\n"); break;
                }
                printf("  输入密码: ");
                scanf("%31s", pass); getchar();
                UserRole roles[] = {USER_ADMIN, USER_OPERATOR, USER_CUSTOMER};
                if (user_db_add(db, name, pass, roles[role_choice - 1], plate)) {
                    printf("  用户 %s 添加成功！\n", name);
                    save_users(db, "data/users.dat");
                } else
                    printf("  添加失败！\n");
                break;
            }
            case 2: {
                char name[32];
                printf("  输入要删除的用户名: ");
                scanf("%31s", name); getchar();
                if (user_db_delete(db, name)) {
                    printf("  用户 %s 已删除。\n", name);
                    save_users(db, "data/users.dat");
                } else
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
                    printf("  %-15s | %s\n", db->users[i].username, rnames[db->users[i].role]);
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
   UI — 各功能界面
   ═══════════════════════════════════════════ */

void ui_entry(ParkingSystem *ps) {
    char plate[16];
    printf("请输入车牌号: ");
    scanf("%15s", plate); getchar();

    switch (vehicle_entry(ps, plate)) {
        case ENTRY_SUCCESS:
            printf("车辆 %s 入场登记成功！\n", plate); break;
        case PARKING_FULL:
            printf("车位已满，车辆 %s 已加入等待队列。\n", plate); break;
        case PLATE_EXISTS:
            printf("车牌 %s 已存在（可能已入场或在队列中），无法重复登记！\n", plate); break;
    }
}

void ui_exit(ParkingSystem *ps) {
    char plate[16];
    double fee = 0.0;
    printf("请输入车牌号: ");
    scanf("%15s", plate); getchar();

    switch (vehicle_exit(ps, plate, &fee)) {
        case PARK_EXIT_SUCCESS:
            printf("车辆 %s 出场成功，费用: %.2f 元\n", plate, fee); break;
        case NOT_FOUND:
            printf("未找到车辆 %s，可能车牌输入有误或车辆不在场内。\n", plate); break;
        case NOT_PARKED:
            printf("车辆 %s 在等待队列中，尚未入场，无法结算。\n", plate); break;
    }
}

void ui_find(ParkingSystem *ps) {
    char plate[16];
    printf("请输入车牌号: ");
    scanf("%15s", plate); getchar();

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
        if (floor_free == 0) printf("无");
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
        scanf("%d", &choice); getchar();

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

/* 自动保存：每次操作后调用，确保数据不丢失 */
static void auto_save(ParkingSystem *ps) {
    save_data(ps, "data/parking_data.dat");
    save_users(&ps->user_db, "data/users.dat");
}

void ui_load(ParkingSystem *ps) {
    if (ps == NULL) return;

    if (ps->history != NULL) { free(ps->history); ps->history = NULL; }
    if (ps->free_spots.spots != NULL) {
        free(ps->free_spots.spots);
        ps->free_spots.spots = NULL;
    }
    while (!queue_is_empty(&ps->wait_queue)) {
        char tmp_plate[16]; time_t tmp_t;
        queue_dequeue(&ps->wait_queue, tmp_plate, &tmp_t);
    }
    ht_clear(&ps->plate_index);

    if (load_data(ps, "data/parking_data.dat"))
        rebuild_hash_from_spots(ps);
    else
        printf("未找到数据文件，使用初始状态。\n");
}

/* ═══════════════════════════════════════════
   UI — 角色菜单
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
        scanf("%d", &choice); getchar();

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
                printf("感谢使用，正在保存数据...\n"); break;
            default:
                printf("无效选择，请重新输入！\n");
        }
        if (choice != 0) auto_save(ps);
    } while (choice != 0);
}

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
        scanf("%d", &choice); getchar();

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
                printf("感谢使用，正在保存数据...\n"); break;
            default:
                printf("无效选择，请重新输入！\n");
        }
        if (choice != 0) auto_save(ps);
    } while (choice != 0);
}

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
        scanf("%d", &choice); getchar();

        switch (choice) {
            case 1: {
                VehicleRecord *rec = find_vehicle(ps, my_plate);
                if (rec) {
                    char ts[64];
                    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S",
                             localtime(&rec->entry_time));
                    int dur = (int)(difftime(time(NULL), rec->entry_time) / 60.0);
                    if (dur < 0) dur = 0;
                    double fee = calculate_fee(&ps->fee_rule, dur);
                    printf("车辆 %s 在场内\n位置：%dF-%d排-%d列\n入场时间：%s\n",
                           my_plate, rec->loc.floor+1, rec->loc.row+1,
                           rec->loc.col+1, ts);
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
                printf("感谢使用，再见！\n"); break;
            default:
                printf("无效选择，请重新输入！\n");
        }
        if (choice != 0) auto_save(ps);
    } while (choice != 0);
}

/* ═══════════════════════════════════════════
   文件 I/O — 停车场数据
   ═══════════════════════════════════════════ */

int save_data(ParkingSystem *ps, const char *filename) {
    if (ps == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("无法打开文件 %s 写入\n", filename);
        return 0;
    }

    FileHeader header;
    header.magic         = FILE_MAGIC;
    header.version       = FILE_VERSION;
    header.history_count = ps->history_count;
    header.free_count    = ps->free_spots.top + 1;
    header.queue_count   = ps->wait_queue.count;
    header.total_revenue = ps->total_revenue;
    header.fee_rule      = ps->fee_rule;
    fwrite(&header, sizeof(FileHeader), 1, fp);

    fwrite(ps->spots, sizeof(ParkingSpot), FLOORS * ROWS * COLS, fp);
    fwrite(ps->free_spots.spots, sizeof(SpotLocation),
           ps->free_spots.top + 1, fp);

    QueueNode *p = ps->wait_queue.front;
    while (p != NULL) {
        fwrite(p, sizeof(QueueNode), 1, fp);
        p = p->next;
    }

    fwrite(ps->history, sizeof(TransactionRecord), ps->history_count, fp);

    fclose(fp);
    return 1;
}

int load_data(ParkingSystem *ps, const char *filename) {
    if (ps == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return 0;

    FileHeader header;
    if (fread(&header, sizeof(FileHeader), 1, fp) != 1) {
        fclose(fp); return 0;
    }

    if (header.magic != FILE_MAGIC) {
        printf("文件格式错误！\n");
        fclose(fp); return 0;
    }

    fread(ps->spots, sizeof(ParkingSpot), FLOORS * ROWS * COLS, fp);

    stack_init(&ps->free_spots, TOTAL_SPOTS);
    ps->free_spots.top = header.free_count - 1;
    if (header.free_count > 0)
        fread(ps->free_spots.spots, sizeof(SpotLocation), header.free_count, fp);

    queue_init(&ps->wait_queue);
    for (int i = 0; i < header.queue_count; i++) {
        QueueNode node;
        if (fread(&node, sizeof(QueueNode), 1, fp) != 1) break;
        queue_enqueue(&ps->wait_queue, node.plate, node.arrival_time);
    }

    ps->history_count = header.history_count;
    ps->history_capacity = header.history_count + 100;
    ps->history = (TransactionRecord *)malloc(
        sizeof(TransactionRecord) * ps->history_capacity);
    if (ps->history == NULL) {
        printf("内存分配失败！\n");
        fclose(fp); return 0;
    }
    if (header.history_count > 0)
        fread(ps->history, sizeof(TransactionRecord), header.history_count, fp);

    ps->total_revenue = header.total_revenue;
    ps->fee_rule = header.fee_rule;

    fclose(fp);
    printf("数据已从 %s 加载\n", filename);
    return 1;
}

void rebuild_hash_from_spots(ParkingSystem *ps) {
    if (ps == NULL) return;
    ht_clear(&ps->plate_index);
    ht_init(&ps->plate_index);

    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED) {
                    VehicleRecord *rec = (VehicleRecord *)malloc(sizeof(VehicleRecord));
                    if (rec == NULL) {
                        printf("内存分配失败！\n"); return;
                    }
                    strcpy(rec->plate, ps->spots[f][r][c].plate);
                    rec->loc = make_loc(f, r, c);
                    rec->entry_time = ps->spots[f][r][c].entry_time;
                    ht_insert(&ps->plate_index, rec);
                }
}

/* ═══════════════════════════════════════════
   文件 I/O — 用户数据
   ═══════════════════════════════════════════ */

int save_users(UserDatabase *db, const char *filename) {
    if (db == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("无法保存用户数据到 %s\n", filename);
        return 0;
    }

    if (fwrite(&db->count, sizeof(int), 1, fp) != 1 ||
        (db->count > 0 && fwrite(db->users, sizeof(User), db->count, fp) != (size_t)db->count)) {
        printf("写入用户数据失败！\n");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

int load_users(UserDatabase *db, const char *filename) {
    if (db == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return 0;  /* 首次运行，文件不存在 */

    int count;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        printf("读取用户文件头失败！\n");
        fclose(fp); return 0;
    }

    if (count < 0 || count > 10000) {
        printf("用户数据损坏（异常数量: %d）！\n", count);
        fclose(fp); return 0;
    }

    while (db->capacity < count) {
        int new_cap = db->capacity > 0 ? db->capacity * 2 : 8;
        User *new_users = (User *)realloc(db->users, sizeof(User) * new_cap);
        if (new_users == NULL) { fclose(fp); return 0; }
        db->users = new_users;
        db->capacity = new_cap;
    }

    size_t read_count = fread(db->users, sizeof(User), count, fp);
    if (read_count != (size_t)count) {
        printf("读取用户数据不完整 (期望 %d, 实际 %zu)！\n", count, read_count);
        fclose(fp); return 0;
    }
    db->count = count;

    fclose(fp);
    printf("已加载 %d 个用户账号\n", count);
    return 1;
}

/* ═══════════════════════════════════════════
   控制台编码配置 & 主入口
   ═══════════════════════════════════════════ */

#ifndef NO_MAIN
static void configure_console_encoding(void) {
#ifdef _WIN32
    SetConsoleOutputCP(936);
    SetConsoleCP(936);
#endif
}

int main(void) {
    ParkingSystem ps;
    UserRole role;
    char username[32];
    char my_plate[16] = "";

    configure_console_encoding();

    /* 0. 显示工作目录（方便排查文件路径问题） */
#ifdef _WIN32
    {
        char cwd[512];
        if (GetCurrentDirectoryA(sizeof(cwd), cwd))
            printf("工作目录: %s\n", cwd);
    }
#endif

    /* 1. 初始化系统 */
    printf("正在初始化停车场系统...\n");
    ds_init(&ps);

    /* 2. 加载用户；若无文件则创建默认管理员 */
    if (!load_users(&ps.user_db, "data/users.dat")) {
        user_db_add(&ps.user_db, "admin", "admin123", USER_ADMIN, "");
        printf("已创建默认管理员账号 (admin / admin123)\n");
    }

    /* 3. 登录 */
    if (!ui_login(&ps.user_db, &role, username)) {
        printf("登录失败，程序退出。\n");
        save_users(&ps.user_db, "data/users.dat");
        ds_destroy(&ps);
        return 1;
    }

    /* 4. 获取车主绑定的车牌 */
    if (role == USER_CUSTOMER) {
        for (int i = 0; i < ps.user_db.count; i++) {
            if (strcmp(ps.user_db.users[i].username, username) == 0) {
                strcpy(my_plate, ps.user_db.users[i].plate);
                break;
            }
        }
    }

    /* 5. 加载停车场数据 */
    if (load_data(&ps, "data/parking_data.dat")) {
        rebuild_hash_from_spots(&ps);
    }

    printf("停车场初始化完成！共 %d 个车位\n", TOTAL_SPOTS);
    printf("当前用户: %s  ", username);

    /* 6. 根据角色进入对应菜单 */
    switch (role) {
        case USER_ADMIN:
            printf("[管理员 — 全部功能]\n");
            ui_admin_menu(&ps);
            break;
        case USER_OPERATOR:
            printf("[操作员 — 出入场管理]\n");
            ui_operator_menu(&ps);
            break;
        case USER_CUSTOMER:
            printf("[车主 — %s]\n", my_plate);
            ui_customer_menu(&ps, my_plate);
            break;
        default:
            printf("[未知角色]\n");
            break;
    }

    /* 7. 退出前保存 */
    printf("正在保存数据...\n");
    save_data(&ps, "data/parking_data.dat");
    save_users(&ps.user_db, "data/users.dat");

    /* 8. 释放所有资源 */
    ds_destroy(&ps);
    printf("系统已安全退出\n");

    return 0;
}
#endif
