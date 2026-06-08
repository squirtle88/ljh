#ifndef DS_H
#define DS_H

#include <time.h>

#define FLOORS 3
#define ROWS 4
#define COLS 5
#define TOTAL_SPOTS (FLOORS * ROWS * COLS)
#define HT_SIZE 10007

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

typedef struct {
    int floor;
    int row;
    int col;
} SpotLocation;

typedef struct {
    SpotLocation loc;
    SpotStatus status;
    char plate[16];
} ParkingSpot;

typedef struct {
    char plate[16];
    SpotLocation loc;
    time_t entry_time;
} VehicleRecord;

typedef struct {
    SpotLocation *spots;
    int top;
    int capacity;
} FreeSpotStack;

typedef struct QueueNode {
    char plate[16];
    time_t arrival_time;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int count;
} WaitingQueue;

typedef struct HTNode {
    char plate[16];
    VehicleRecord *record;
    struct HTNode *next;
} HTNode;

typedef struct {
    HTNode *buckets[HT_SIZE];
    int count;
} HashMap;

typedef struct {
    double base_fee;
    int free_minutes;
    double rate_per_hour;
    double max_daily;
} FeeRule;

typedef struct {
    char plate[16];
    SpotLocation loc;
    time_t entry_time;
    time_t exit_time;
    int duration_minutes;
    double fee;
} TransactionRecord;

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

void ds_init(ParkingSystem *ps);
void ds_destroy(ParkingSystem *ps);

void stack_init(FreeSpotStack *s, int capacity);
void stack_destroy(FreeSpotStack *s);
void stack_push(FreeSpotStack *s, SpotLocation loc);
SpotLocation stack_pop(FreeSpotStack *s);
int stack_is_empty(const FreeSpotStack *s);
int stack_size(const FreeSpotStack *s);

void queue_init(WaitingQueue *q);
void queue_clear(WaitingQueue *q);
void queue_enqueue(WaitingQueue *q, const char *plate, time_t t);
int queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t);
int queue_is_empty(const WaitingQueue *q);
int queue_count(const WaitingQueue *q);

unsigned int hash_str(const char *str);
void ht_init(HashMap *ht);
void ht_insert(HashMap *ht, VehicleRecord *rec);
VehicleRecord *ht_find(HashMap *ht, const char *plate);
void ht_remove(HashMap *ht, const char *plate);
void ht_clear(HashMap *ht);

SpotLocation make_loc(int f, int r, int c);
int loc_equal(SpotLocation a, SpotLocation b);

#endif
