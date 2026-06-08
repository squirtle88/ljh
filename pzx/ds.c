#include "ds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void copy_plate(char dest[16], const char *src) {
    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    strncpy(dest, src, 15);
    dest[15] = '\0';
}

SpotLocation make_loc(int f, int r, int c) {
    SpotLocation loc;
    loc.floor = f;
    loc.row = r;
    loc.col = c;
    return loc;
}

int loc_equal(SpotLocation a, SpotLocation b) {
    return a.floor == b.floor && a.row == b.row && a.col == b.col;
}

void stack_init(FreeSpotStack *s, int capacity) {
    if (s == NULL || capacity <= 0) {
        return;
    }

    s->spots = (SpotLocation *)malloc(sizeof(SpotLocation) * capacity);
    if (s->spots == NULL) {
        fprintf(stderr, "stack_init: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    s->top = -1;
    s->capacity = capacity;
}

void stack_destroy(FreeSpotStack *s) {
    if (s == NULL) {
        return;
    }

    free(s->spots);
    s->spots = NULL;
    s->top = -1;
    s->capacity = 0;
}

void stack_push(FreeSpotStack *s, SpotLocation loc) {
    if (s == NULL) {
        return;
    }

    if (s->top + 1 >= s->capacity) {
        int new_capacity = s->capacity > 0 ? s->capacity * 2 : 1;
        SpotLocation *new_spots = (SpotLocation *)realloc(
            s->spots, sizeof(SpotLocation) * new_capacity);
        if (new_spots == NULL) {
            fprintf(stderr, "stack_push: stack expansion failed\n");
            exit(EXIT_FAILURE);
        }

        s->spots = new_spots;
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

int stack_is_empty(const FreeSpotStack *s) {
    return s == NULL || s->top == -1;
}

int stack_size(const FreeSpotStack *s) {
    if (s == NULL) {
        return 0;
    }

    return s->top + 1;
}

void queue_init(WaitingQueue *q) {
    if (q == NULL) {
        return;
    }

    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
}

void queue_clear(WaitingQueue *q) {
    if (q == NULL) {
        return;
    }

    while (!queue_is_empty(q)) {
        char plate[16];
        time_t t;
        queue_dequeue(q, plate, &t);
    }
}

void queue_enqueue(WaitingQueue *q, const char *plate, time_t t) {
    if (q == NULL) {
        return;
    }

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
        q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }

    q->count++;
}

int queue_dequeue(WaitingQueue *q, char *out_plate, time_t *out_t) {
    if (q == NULL || queue_is_empty(q)) {
        return 0;
    }

    QueueNode *old_front = q->front;
    if (out_plate != NULL) {
        copy_plate(out_plate, old_front->plate);
    }
    if (out_t != NULL) {
        *out_t = old_front->arrival_time;
    }

    q->front = old_front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(old_front);
    q->count--;
    return 1;
}

int queue_is_empty(const WaitingQueue *q) {
    return q == NULL || q->front == NULL || q->count == 0;
}

int queue_count(const WaitingQueue *q) {
    if (q == NULL) {
        return 0;
    }

    return q->count;
}

unsigned int hash_str(const char *str) {
    unsigned long hash = 0;

    if (str == NULL) {
        return 0;
    }

    while (*str != '\0') {
        hash = hash * 31 + (unsigned char)(*str++);
    }

    return (unsigned int)(hash % HT_SIZE);
}

void ht_init(HashMap *ht) {
    if (ht == NULL) {
        return;
    }

    for (int i = 0; i < HT_SIZE; i++) {
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}

void ht_insert(HashMap *ht, VehicleRecord *rec) {
    if (ht == NULL || rec == NULL) {
        return;
    }

    unsigned int idx = hash_str(rec->plate);
    HTNode *node = (HTNode *)malloc(sizeof(HTNode));
    if (node == NULL) {
        fprintf(stderr, "ht_insert: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    copy_plate(node->plate, rec->plate);
    node->record = rec;
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->count++;
}

VehicleRecord *ht_find(HashMap *ht, const char *plate) {
    if (ht == NULL || plate == NULL) {
        return NULL;
    }

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
    if (ht == NULL || plate == NULL) {
        return;
    }

    unsigned int idx = hash_str(plate);
    HTNode *p = ht->buckets[idx];
    HTNode *prev = NULL;

    while (p != NULL) {
        if (strcmp(p->plate, plate) == 0) {
            if (prev == NULL) {
                ht->buckets[idx] = p->next;
            } else {
                prev->next = p->next;
            }
            free(p);
            ht->count--;
            return;
        }

        prev = p;
        p = p->next;
    }
}

void ht_clear(HashMap *ht) {
    if (ht == NULL) {
        return;
    }

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

void ds_init(ParkingSystem *ps) {
    if (ps == NULL) {
        return;
    }

    for (int f = 0; f < FLOORS; f++) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                ps->spots[f][r][c].loc = make_loc(f, r, c);
                ps->spots[f][r][c].status = SPOT_FREE;
                ps->spots[f][r][c].plate[0] = '\0';
            }
        }
    }

    stack_init(&ps->free_spots, TOTAL_SPOTS);
    for (int f = 0; f < FLOORS; f++) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                stack_push(&ps->free_spots, make_loc(f, r, c));
            }
        }
    }

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

    ps->fee_rule.base_fee = 5.0;
    ps->fee_rule.free_minutes = 15;
    ps->fee_rule.rate_per_hour = 4.0;
    ps->fee_rule.max_daily = 40.0;
    ps->total_revenue = 0.0;
}

void ds_destroy(ParkingSystem *ps) {
    if (ps == NULL) {
        return;
    }

    stack_destroy(&ps->free_spots);
    ht_clear(&ps->plate_index);
    queue_clear(&ps->wait_queue);

    free(ps->history);
    ps->history = NULL;
    ps->history_count = 0;
    ps->history_capacity = 0;
}
